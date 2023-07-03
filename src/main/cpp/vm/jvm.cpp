#include "jvm.hpp"

#include <memory>
#include <utility>
#include "../utils/log.h"

namespace kotlin_tracer {

JVM::JVM(
    std::shared_ptr<JavaVM> java_vm,
    jvmtiEventCallbacks *callbacks
) : java_vm_(std::move(java_vm)), threads_(std::make_shared<ConcurrentList<std::shared_ptr<ThreadInfo>>>()) {
  jvmtiEnv *pJvmtiEnv = nullptr;
  this->java_vm_->GetEnv((void **) &pJvmtiEnv, JVMTI_VERSION_11);
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
}

void JVM::addCurrentThread(::jthread thread) {
  JNIEnv *env_id;
  java_vm_->GetEnv((void **) &env_id, JNI_VERSION_10);
  pthread_t currentThread = pthread_self();
  jvmtiThreadInfo info{};
  auto err = jvmti_env_->GetThreadInfo(thread, &info);
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
  java_vm_->GetEnv((void **) &env, JNI_VERSION_10);
  return env;
}

void JVM::attachThread() {
  JNIEnv *pEnv = getJNIEnv();
  java_vm_->AttachCurrentThreadAsDaemon((void **) &pEnv, nullptr);
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
}