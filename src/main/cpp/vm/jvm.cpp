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
) : java_vm_(std::move(java_vm)), threads_(std::make_shared<ConcurrentList<std::shared_ptr<ThreadInfo>>>()),
    addr_2_symbol_() {
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
  init(&addr_2_symbol_);
  resolveVMStructs();
  resolveVMTypes();
  jvm_code_cache_ = std::make_unique<JVMCodeCache>(types_);
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
  auto threadInfo = std::make_shared<ThreadInfo>(name, currentThread);
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
    field->fieldName = *reinterpret_cast<char **>(entry + gHotSpotVMStructEntryFieldNameOffset);
    char *typeName = *reinterpret_cast<char **>(entry + gHotSpotVMStructEntryTypeNameOffset);
    if (typeName == nullptr || field->fieldName == nullptr) break;
    string typeNameStr = string(typeName);
    int32_t isStatic = *reinterpret_cast<int32_t *>(entry + gHotSpotVMStructEntryIsStaticOffset);
    uint64_t offset = *reinterpret_cast<uint64_t *>(entry + gHotSpotVMStructEntryOffsetOffset);
    uint64_t address = *reinterpret_cast<uint64_t *>(entry + gHotSpotVMStructEntryAddressOffset);
    field->typeString = *reinterpret_cast<char **>(entry + gHotSpotVMStructEntryTypeStringOffset);
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
    vmTypeEntry.typeName = *reinterpret_cast<char **>(entry + gHotSpotVMTypeEntryTypeNameOffset);
    if (vmTypeEntry.typeName == nullptr) break;
    vmTypeEntry.superclassName = *reinterpret_cast<char **>(entry + gHotSpotVMTypeEntrySuperclassNameOffset);
    vmTypeEntry.isOopType = *reinterpret_cast<int32_t *>(entry + gHotSpotVMTypeEntryIsOopTypeOffset);
    vmTypeEntry.isIntegerType = *reinterpret_cast<int32_t *>(entry + gHotSpotVMTypeEntryIsIntegerTypeOffset);
    vmTypeEntry.isUnsigned = *reinterpret_cast<int32_t *>(entry + gHotSpotVMTypeEntryIsUnsignedOffset);
    vmTypeEntry.size = *reinterpret_cast<int64_t *>(entry + gHotSpotVMTypeEntrySizeOffset);
    vmTypeEntry.fields = vm_structs_[vmTypeEntry.typeName];
    types_[vmTypeEntry.typeName] = vmTypeEntry;
  }
}

bool JVM::isJavaFrame(uint64_t address) {
  return jvm_code_cache_->isJavaFrame(address);
}

const addr2Symbol::function_info *JVM::getNativeFunctionInfo(intptr_t address) {
  return addr_2_symbol_.getFunctionInfo(address);
}

jmethodID JVM::getJMethodId(uint64_t address, uint64_t frame_pointer) {
  return jvm_code_cache_->getJMethodId(address, frame_pointer);
}
}  // namespace kotlin_tracer
