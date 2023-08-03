#include "jvmCodeCache.h"

#include <string>
#include "../utils/pointerValidation.h"

namespace kotlin_tracer {
bool JVMCodeCache::isJavaFrame(uint64_t instruction_pointer) {
  auto len = *heaps_length;
  for (int i = 0; i < len; ++i) {
    auto heap_ptr = heaps_data[i];
    auto memory_ptr = heap_ptr + memoryField->offset;
    auto low = *reinterpret_cast<uint64_t *>(memory_ptr + lowField->offset);
    auto high = *reinterpret_cast<uint64_t *>(memory_ptr + highField->offset);
    if (low <= instruction_pointer && instruction_pointer < high) {
      return true;
    }
  }
  return false;
}

jmethodID JVMCodeCache::getJMethodId(uint64_t instruction_pointer, uint64_t frame_pointer) {
  auto len = *heaps_length;
  for (int i = 0; i < len; ++i) {
    auto heap_ptr = heaps_data[i];
    auto memory_ptr = heap_ptr + memoryField->offset;
    auto segmap_ptr = heap_ptr + segmapField->offset;
    auto _log2_segment_size = *reinterpret_cast<int *>(heap_ptr + log2SegmentSizeField->offset);
    auto low = *reinterpret_cast<uint64_t *>(memory_ptr + lowField->offset);
    auto high = *reinterpret_cast<uint64_t *>(memory_ptr + highField->offset);
    if (low <= instruction_pointer && instruction_pointer < high) {
      auto seg_map = *reinterpret_cast<address *>(segmap_ptr + lowField->offset);
      size_t seg_idx = (instruction_pointer - low) >> _log2_segment_size;

      // Iterate the segment map chain to find the start of the block.
      while (seg_map[seg_idx] > 0) {
        // Don't check each segment index to refer to a used segment.
        // This method is called extremely often. Therefore, any checking
        // has a significant impact on performance. Rely on CodeHeap::verify()
        // to do the job on request.
        seg_idx -= static_cast<int>(seg_map[seg_idx]);
      }
      auto heapBlockAddr = (uint64_t) (low + (seg_idx << _log2_segment_size));
      heapBlockAddr += heap_block_type_size;
      auto codeBlobName = *reinterpret_cast<char **>(heapBlockAddr + codeBlobNameField->offset);
      const auto &codeBlobNameStr = std::string(codeBlobName);
      if (codeBlobNameStr == "Interpreter") {
        auto *fp_ptr = reinterpret_cast<intptr_t *>(frame_pointer);
        intptr_t method_ptr = fp_ptr[-3];
        if (method_ptr == 0) return nullptr;
        auto constMethod_ptr = *reinterpret_cast<uint64_t *>(method_ptr + constMethodField->offset);
        auto idnum_ptr_offset = constMethod_ptr + methodIdNumField->offset;
        if (!is_valid(idnum_ptr_offset)) return nullptr;
        auto idnum = *reinterpret_cast<u2 *>(idnum_ptr_offset);
        auto constantPool_ptr = *reinterpret_cast<uint64_t *>(constMethod_ptr + constantsField->offset);
        uint64_t poolHandler_ptr_offset = constantPool_ptr + poolHolderField->offset;
        if (!is_valid(poolHandler_ptr_offset)) return nullptr;
        auto poolHolder_ptr = *reinterpret_cast<uint64_t *>(poolHandler_ptr_offset);
        std::atomic_thread_fence(std::memory_order_acquire);
        auto jmethodIds = *reinterpret_cast<jmethodID **>(poolHolder_ptr + jmethodIdsField->offset);
        jmethodID id = nullptr;
        if (jmethodIds != nullptr &&                         // If there is a cache
            (reinterpret_cast<size_t>(jmethodIds[0])) > idnum) {   // and if it is long enough,
          id = jmethodIds[idnum + 1];                       // Look up the id (may be NULL)
        }
        return id;
      } else if (codeBlobNameStr.contains("nmethod")) {
        auto method_ptr = *reinterpret_cast<uint64_t *>(heapBlockAddr + compiledMethodFiled->offset);
        auto constMethod_ptr = *reinterpret_cast<uint64_t *>(method_ptr + constMethodField->offset);
        auto idnum = *reinterpret_cast<u2 *>(constMethod_ptr + methodIdNumField->offset);
        auto constantPool_ptr = *reinterpret_cast<uint64_t *>(constMethod_ptr + constantsField->offset);
        std::atomic_thread_fence(std::memory_order_acquire);
        auto poolHolder_ptr = *reinterpret_cast<uint64_t *>(constantPool_ptr + poolHolderField->offset);
        auto jmethodIds = *reinterpret_cast<jmethodID **>(poolHolder_ptr + jmethodIdsField->offset);
        jmethodID id = nullptr;
        if (jmethodIds != nullptr &&                         // If there is a cache
            (reinterpret_cast<size_t>(jmethodIds[0])) > idnum) {   // and if it is long enough,
          id = jmethodIds[idnum + 1];                       // Look up the id (may be NULL)
        }
        return id;
      }
    }
  }
  return nullptr;
}
}  // namespace kotlin_tracer
