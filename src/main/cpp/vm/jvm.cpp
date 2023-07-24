#include <memory>
#include <utility>
#include <string>

#include "jvm.hpp"
#include "../utils/log.h"
#include "vmStructs.h"

namespace kotlin_tracer {
using std::shared_ptr, std::string, std::unordered_map;
JVM::JVM(
    std::shared_ptr<JavaVM> java_vm,
    jvmtiEventCallbacks *callbacks
) : java_vm_(std::move(java_vm)), threads_(std::make_shared<ConcurrentList<std::shared_ptr<ThreadInfo>>>()) {
  jvmtiEnv *pJvmtiEnv = nullptr;
  this->java_vm_->GetEnv(reinterpret_cast<void **>(&pJvmtiEnv), JVMTI_VERSION_11);
  jvmti_env_ = pJvmtiEnv;
  jvmtiCapabilities capabilities;
  capabilities.can_generate_garbage_collection_events = 1;
  capabilities.can_generate_all_class_hook_events = 1;
  jvmti_env_->AddCapabilities(&capabilities);
  jvmti_env_->SetEventCallbacks(callbacks, sizeof(*callbacks));
  jvmti_env_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH,
                                       nullptr);
  jvmti_env_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START,
                                       nullptr);
  jvmti_env_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_START,
                                       nullptr);
  jvmti_env_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT,
                                       nullptr);
  jvmti_env_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_COMPILED_METHOD_LOAD,
                                       nullptr);
  jvmti_env_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_START,
                                       nullptr);
  jvmti_env_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_LOAD,
                                       nullptr);
  jvmti_env_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_PREPARE,
                                       nullptr);
  jvmti_env_->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK,
                                       nullptr);
  resolveVMStructs();
  resolveVMTypes();
}

void JVM::addCurrentThread(::jthread thread) {
  JNIEnv *env_id;
  java_vm_->GetEnv(reinterpret_cast<void **>(&env_id), JNI_VERSION_10);
  pthread_t currentThread = pthread_self();
  jvmtiThreadInfo info{};
  auto err = jvmti_env_->GetThreadInfo(thread, &info);
  logDebug("Added thread: " + std::string(info.name));
  if (err != JVMTI_ERROR_NONE) {
    logError("Failed to get thead info:" + std::to_string(err));
    return;
  }
  auto name = std::make_shared<std::string>(info.name);
  auto threadInfo = std::make_shared<ThreadInfo>(ThreadInfo{name, currentThread});
  threads_->push_back(threadInfo);
}

std::shared_ptr<ConcurrentList<std::shared_ptr<ThreadInfo>>> JVM::getThreads() {
  return threads_;
}

std::shared_ptr<ThreadInfo> JVM::findThread(const pthread_t &thread) {
  std::function<bool(std::shared_ptr<ThreadInfo>)>
      findThread = [thread](const std::shared_ptr<ThreadInfo> &threadInfo) {
    return pthread_equal(threadInfo->id, thread);
  };
  return threads_->find(findThread);
}

jvmtiEnv *JVM::getJvmTi() {
  return jvmti_env_;
}

JNIEnv *JVM::getJNIEnv() {
  JNIEnv *env;
  java_vm_->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_10);
  return env;
}

void JVM::attachThread() {
  JNIEnv *pEnv = getJNIEnv();
  java_vm_->AttachCurrentThreadAsDaemon(reinterpret_cast<void **>(&pEnv), nullptr);
}

void JVM::dettachThread() {
  java_vm_->DetachCurrentThread();
}

void JVM::initializeMethodIds(jvmtiEnv *jvmti_env, JNIEnv *jni_env) {
  jint counter = 0;
  jclass *classes;
  jvmtiError err = jvmti_env->GetLoadedClasses(&counter, &classes);
  if (err != JVMTI_ERROR_NONE) {
    printf("Failed to load classes\n");
    fflush(stdout);
  }
  for (int i = 0; i < counter; i++) {
    jclass klass = (classes)[i];
    loadMethodsId(jvmti_env, jni_env, klass);
  }
  jvmti_env->Deallocate((unsigned char *) classes);
}

// The jmethodIDs should be allocated. Or we'll get 0 method id
void JVM::loadMethodsId(jvmtiEnv *jvmti_env, __attribute__((unused)) JNIEnv *jni_env, jclass klass) {
  jint method_count = 0;
  jmethodID *methods = nullptr;
  if (jvmti_env->GetClassMethods(klass, &method_count, &methods) == 0) {
    jvmti_env->Deallocate((unsigned char *) methods);
  }
}

void JVM::resolveVMStructs() {
  char *entry = gHotSpotVMStructs;
  for (;; entry += gHotSpotVMStructEntryArrayStride) {
    shared_ptr<Field> field(new Field());
    field->fieldName = *(char **) (entry + gHotSpotVMStructEntryFieldNameOffset);
    char *typeName = *(char **) (entry + gHotSpotVMStructEntryTypeNameOffset);
    if (typeName == nullptr || field->fieldName == nullptr) break;
    string typeNameStr = string(typeName);
    int32_t isStatic = *(int32_t *) (entry + gHotSpotVMStructEntryIsStaticOffset);
    uint64_t offset = *(uint64_t *) (entry + gHotSpotVMStructEntryOffsetOffset);
    uint64_t address = *(uint64_t *) (entry + gHotSpotVMStructEntryAddressOffset);
    field->typeString = *(char **) (entry + gHotSpotVMStructEntryTypeStringOffset);
    field->isStatic = isStatic;
    if (isStatic) {
      field->offset = address;
    } else {
      field->offset = offset;
    }
    auto search = JVM::vm_structs_.find(typeNameStr);
    shared_ptr<unordered_map<string, shared_ptr<Field>>> fields;
    if (search == JVM::vm_structs_.end()) {
      fields = std::make_shared<unordered_map<string, shared_ptr<Field>>>();
      JVM::vm_structs_[typeNameStr] = fields;
    } else {
      fields = search->second;
    }
    fields->insert({string(field->fieldName), field});
  }
}

void JVM::resolveVMTypes() {
  char *entry = gHotSpotVMTypes;
  for (;; entry += gHotSpotVMTypeEntryArrayStride) {
    VMTypeEntry vmTypeEntry;
    vmTypeEntry.typeName = *(char **) (entry + gHotSpotVMTypeEntryTypeNameOffset);
    if (vmTypeEntry.typeName == nullptr) break;
    vmTypeEntry.superclassName = *(char **) (entry + gHotSpotVMTypeEntrySuperclassNameOffset);
    vmTypeEntry.isOopType = *(int32_t *) (entry + gHotSpotVMTypeEntryIsOopTypeOffset);
    vmTypeEntry.isIntegerType = *(int32_t *) (entry + gHotSpotVMTypeEntryIsIntegerTypeOffset);
    vmTypeEntry.isUnsigned = *(int32_t *) (entry + gHotSpotVMTypeEntryIsUnsignedOffset);
    vmTypeEntry.size = *(int64_t *) (entry + gHotSpotVMTypeEntrySizeOffset);
    vmTypeEntry.fields = vm_structs_[vmTypeEntry.typeName];
    types_[vmTypeEntry.typeName] = vmTypeEntry;
  }
}

typedef u_char*       address;
typedef unsigned short u2;
void JVM::getCodeCache(uint64_t pointer, uint64_t fp) {
  auto compiledMethodType = types_.at("CompiledMethod");
  auto compiledMethodFiled = compiledMethodType.fields->at("_method");
  auto methodType = types_.at("Method");
  auto constMethodField = methodType.fields->at("_constMethod");
  auto constMethodType = types_.at("ConstMethod");
  auto methodIdField = constMethodType.fields->at("_method_idnum");
  auto growableArrayBaseType = types_.at("GrowableArrayBase");
  auto codeBlobType = types_.at("CodeBlob");
  auto codeBlobNameField = codeBlobType.fields->at("_name");
  auto growableArrayType = types_.at("GrowableArray<int>");
  auto codeHeapType = types_.at("CodeHeap");
  auto memoryField = codeHeapType.fields->at("_memory");
  auto segmapField = codeHeapType.fields->at("_segmap");
  auto log2SegmentSizeField = codeHeapType.fields->at("_log2_segment_size");
  auto virtualSpaceType = types_.at("VirtualSpace");
  auto lowField = virtualSpaceType.fields->at("_low");
  auto highField = virtualSpaceType.fields->at("_high");
  auto heaps = *reinterpret_cast<uint64_t *>(types_.at("CodeCache").fields->at("_heaps")->offset);
  auto data = *reinterpret_cast<uint64_t **>(heaps + growableArrayType.fields->at("_data")->offset);
  auto len = *reinterpret_cast<int *>(heaps + growableArrayBaseType.fields->at("_len")->offset);
  if (len > 0) {
    for (int i = 0; i < len; ++i) {
      auto heap_ptr = data[i];
      auto memory_ptr = heap_ptr + memoryField->offset;
      auto segmap_ptr = heap_ptr + segmapField->offset;
      auto _log2_segment_size = *reinterpret_cast<int*>(heap_ptr + log2SegmentSizeField->offset);
      auto low = *reinterpret_cast<uint64_t *>(memory_ptr + lowField->offset);
      auto high = *reinterpret_cast<uint64_t *>(memory_ptr + highField->offset);
      if (low <= pointer && pointer < high) {
        auto seg_map = *reinterpret_cast<address*>(segmap_ptr + lowField->offset);
        size_t  seg_idx = (pointer - low) >> _log2_segment_size;

        // This may happen in special cases. Just ignore.
        // Example: PPC ICache stub generation.
//        if (is_segment_unused(seg_map[seg_idx])) {
//          return NULL;
//        }

        // Iterate the segment map chain to find the start of the block.
        while (seg_map[seg_idx] > 0) {
          // Don't check each segment index to refer to a used segment.
          // This method is called extremely often. Therefore, any checking
          // has a significant impact on performance. Rely on CodeHeap::verify()
          // to do the job on request.
          seg_idx -= (int)seg_map[seg_idx];
        }
        auto heapBlockAddr = (uint64_t)(low + (seg_idx << _log2_segment_size));
        heapBlockAddr += types_.at("HeapBlock").size;
        auto codeBlobName = *(char**)(heapBlockAddr + codeBlobNameField->offset);
        const auto &codeBlobNameStr = string(codeBlobName);
        logDebug("Code blob:" + codeBlobNameStr);
        if (codeBlobNameStr == "Interpreter") {
          auto *fp_ptr = (intptr_t *) fp;
          intptr_t method_ptr = fp_ptr[-3];
          char *pName;
          char *pSignature;
          char *pGeneric;
          jvmti_env_->GetMethodName((jmethodID)(&method_ptr), &pName, &pSignature, &pGeneric);
          logDebug("Method name: " + string(pName));
        } else if (codeBlobNameStr.contains("nmethod")) {
          auto method_ptr = *reinterpret_cast<uint64_t *>(heapBlockAddr + compiledMethodFiled->offset);
          char *pName;
          char *pSignature;
          char *pGeneric;
          jvmti_env_->GetMethodName((jmethodID)(&method_ptr), &pName, &pSignature, &pGeneric);
          logDebug("NMethod name: " + string(pName));
        }

      }
    }
  }
}
}  // namespace kotlin_tracer
