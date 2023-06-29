#include "jvm.hpp"

#include <memory>
#include <utility>
#include "../utils/log.h"

using namespace std;

kotlinTracer::JVM::JVM(shared_ptr<JavaVM> t_vm, jvmtiEventCallbacks *t_callbacks) : m_vm(std::move(t_vm)),
                                                                         m_threads(make_shared<kotlinTracer::ConcurrentList<shared_ptr<ThreadInfo>>>()) {
  jvmtiEnv *pJvmtiEnv = nullptr;
  this->m_vm->GetEnv((void **) &pJvmtiEnv, JVMTI_VERSION_11);
  m_jvmti = pJvmtiEnv;
  jvmtiCapabilities capabilities;
  capabilities.can_generate_garbage_collection_events = 1;
  capabilities.can_generate_all_class_hook_events = 1;
  m_jvmti->AddCapabilities(&capabilities);
  m_jvmti->SetEventCallbacks(t_callbacks, sizeof(*t_callbacks));
  m_jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, nullptr);
  m_jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, nullptr);
  m_jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_START, nullptr);
  m_jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, nullptr);
  m_jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_COMPILED_METHOD_LOAD, nullptr);
  m_jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_START, nullptr);
  m_jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_LOAD, nullptr);
  m_jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_PREPARE, nullptr);
  m_jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, nullptr);
}

kotlinTracer::JVM::~JVM() = default;

void kotlinTracer::JVM::addCurrentThread(::jthread t_thread) {
  JNIEnv *env_id;
  m_vm->GetEnv((void **) &env_id, JNI_VERSION_10);
  pthread_t currentThread = pthread_self();
  jvmtiThreadInfo info{};
  auto err = m_jvmti->GetThreadInfo(t_thread, &info);
  if (err != JVMTI_ERROR_NONE) {
    cout << "Failed to get thead info" << err << endl;
    return;
  }
  auto name = make_shared<string>(info.name);
  auto threadInfo = std::make_shared<ThreadInfo>(ThreadInfo{name, currentThread});
  m_threads->push_back(threadInfo);
}

shared_ptr<kotlinTracer::ConcurrentList<shared_ptr<kotlinTracer::ThreadInfo>>> kotlinTracer::JVM::getThreads() {
  return m_threads;
}

shared_ptr<kotlinTracer::ThreadInfo> kotlinTracer::JVM::findThread(const pthread_t &t_thread) {
  list<shared_ptr<ThreadInfo>>::iterator threadInfo;
  std::function<bool(shared_ptr<ThreadInfo>)> findThread = [t_thread](shared_ptr<ThreadInfo> threadInfo) {
    return pthread_equal(threadInfo->id, t_thread);
  };
  return m_threads->find(findThread);
}

jvmtiEnv* kotlinTracer::JVM::getJvmTi() {
  return m_jvmti;
}

JNIEnv *kotlinTracer::JVM::getJNIEnv() {
  JNIEnv *env;
  m_vm->GetEnv((void **) &env, JNI_VERSION_10);
  return env;
}

void kotlinTracer::JVM::attachThread() {
  JNIEnv *pEnv = getJNIEnv();
  m_vm->AttachCurrentThreadAsDaemon((void **) &pEnv, nullptr);
}

void kotlinTracer::JVM::dettachThread() {
  m_vm->DetachCurrentThread();
}

void kotlinTracer::JVM::initializeMethodIds(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv) {
  jint counter = 0;
  jclass *classes;
  jvmtiError err = t_jvmtiEnv->GetLoadedClasses(&counter, &classes);
  if (err != JVMTI_ERROR_NONE) {
    printf("Failed to load classes\n");
    fflush(stdout);
  }
  for (int i = 0; i < counter; i++) {
    jclass klass = (classes)[i];
    loadMethodsId(t_jvmtiEnv, t_jniEnv, klass);
  }
  t_jvmtiEnv->Deallocate((unsigned char *) classes);
}

// The jmethodIDs should be allocated. Or we'll get 0 method id
void kotlinTracer::JVM::loadMethodsId(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv, jclass t_class) {
  jint method_count = 0;
  jmethodID *methods = nullptr;
  if (t_jvmtiEnv->GetClassMethods(t_class, &method_count, &methods) == 0) {
    t_jvmtiEnv->Deallocate((unsigned char *) methods);
  }
}