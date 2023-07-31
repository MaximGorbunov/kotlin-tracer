#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_VM_JVMCODECACHE_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_VM_JVMCODECACHE_H_

#include <memory>
#include "vmStructs.h"
#include <jvmti.h>

namespace kotlin_tracer {
typedef unsigned char *address;
typedef unsigned short u2;

class JVMCodeCache {
 public:
  explicit JVMCodeCache(const std::unordered_map<std::string, VMTypeEntry> &types) {
    auto const &compiledMethodType = getIfContains(types, "CompiledMethod");
    compiledMethodFiled = getIfContains(*compiledMethodType.fields, "_method");
    const auto &methodType = getIfContains(types, "Method");
    constMethodField = getIfContains(*methodType.fields, "_constMethod");
    const auto &constMethodType = getIfContains(types, "ConstMethod");
    constantsField = getIfContains(*constMethodType.fields, "_constants");
    methodIdNumField = getIfContains(*constMethodType.fields, "_method_idnum");
    const auto &constantPoolType = getIfContains(types, "ConstantPool");
    poolHolderField = getIfContains(*constantPoolType.fields, "_pool_holder");
    const auto &instanceKlassType = getIfContains(types, "InstanceKlass");
    jmethodIdsField = getIfContains(*instanceKlassType.fields, "_methods_jmethod_ids");
    const auto &growableArrayBaseType = getIfContains(types, "GrowableArrayBase");
    const auto &codeBlobType = getIfContains(types, "CodeBlob");
    codeBlobNameField = getIfContains(*codeBlobType.fields, "_name");
    const auto &growableArrayType = getIfContains(types, "GrowableArray<int>");
    const auto &codeHeapType = getIfContains(types, "CodeHeap");
    memoryField = getIfContains(*codeHeapType.fields, "_memory");
    segmapField = getIfContains(*codeHeapType.fields, "_segmap");
    log2SegmentSizeField = getIfContains(*codeHeapType.fields, "_log2_segment_size");
    const auto &virtualSpaceType = getIfContains(types, "VirtualSpace");
    lowField = getIfContains(*virtualSpaceType.fields, "_low");
    highField = getIfContains(*virtualSpaceType.fields, "_high");
    const auto heaps_offset = getIfContains(*getIfContains(types, "CodeCache").fields, "_heaps")->offset;
    const auto data_offset = getIfContains(*growableArrayType.fields, "_data")->offset;
    heaps = *reinterpret_cast<uint64_t *>(heaps_offset);
    heaps_data = *reinterpret_cast<uint64_t **>(heaps + data_offset);
    heaps_length = reinterpret_cast<int *>(heaps + getIfContains(*growableArrayBaseType.fields, "_len")->offset);
    heap_block_type_size = getIfContains(types, "HeapBlock").size;
  }

  bool isJavaFrame(uint64_t address);
  jmethodID getJMethodId(uint64_t address, uint64_t frame_pointer);

 private:
  std::shared_ptr<Field> compiledMethodFiled;
  std::shared_ptr<Field> constMethodField;
  std::shared_ptr<Field> constantsField;
  std::shared_ptr<Field> methodIdNumField;
  std::shared_ptr<Field> poolHolderField;
  std::shared_ptr<Field> jmethodIdsField;
  std::shared_ptr<Field> codeBlobNameField;
  std::shared_ptr<Field> memoryField;
  std::shared_ptr<Field> segmapField;
  std::shared_ptr<Field> log2SegmentSizeField;
  std::shared_ptr<Field> lowField;
  std::shared_ptr<Field> highField;
  uint64_t heaps;
  uint64_t *heaps_data;
  int *heaps_length;
  uint64_t heap_block_type_size;
  template<typename T>
  inline T getIfContains(const std::unordered_map<std::string, T> &map, const std::string &key) {
    if (map.contains(key)) {
      return map.at(key);
    }
    throw std::runtime_error(key + " not found in vm structs");
  }
};
} // kotlin_tracer
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_VM_JVMCODECACHE_H_
