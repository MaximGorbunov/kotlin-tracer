#include "jvm.h"

#include <memory>
#include "log.h"

using namespace std;

shared_ptr<JVM> JVM::instance = shared_ptr<JVM>();

JVM::JVM(JavaVM *vm, jvmtiEventCallbacks *callbacks) : vm(vm), threadListMutex(),
                                                       threads(new list<shared_ptr<threadInfo_t>>()) {
  jvmtiEnv *pJvmtiEnv = nullptr;
  this->vm->GetEnv((void **) &pJvmtiEnv, JVMTI_VERSION_11);
  jvmti = shared_ptr<jvmtiEnv>(pJvmtiEnv);
  jvmtiCapabilities capabilities;
  capabilities.can_generate_garbage_collection_events = 1;
  capabilities.can_generate_all_class_hook_events = 1;
  jvmti->AddCapabilities(&capabilities);
  jvmti->SetEventCallbacks(callbacks, sizeof(*callbacks));
  jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, nullptr);
  jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, nullptr);
  jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_START, nullptr);
  jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, nullptr);
  jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_COMPILED_METHOD_LOAD, nullptr);
  jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_START, nullptr);
  jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_LOAD, nullptr);
  jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_PREPARE, nullptr);
  jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, nullptr);
}

JVM::~JVM() {
  instance.reset();
  jvmti->DisposeEnvironment();
}

shared_ptr<JVM> JVM::createInstance(JavaVM *vm, jvmtiEventCallbacks *callbacks) {
  if (instance == nullptr) {
    instance.reset(new JVM(vm, callbacks));
  }
  return instance;
}

shared_ptr<JVM> JVM::getInstance() {
  return instance;
}

void JVM::addCurrentThread(jthread thread) {
  JNIEnv *env_id;
  vm->GetEnv((void **) &env_id, JNI_VERSION_10);
  pthread_t currentThread = pthread_self();
  jvmtiThreadInfo info{};
  auto err = jvmti->GetThreadInfo(thread, &info);
  if (err != JVMTI_ERROR_NONE) {
    cout << "Failed to get thead info" << err << endl;
    return;
  }
  auto name = make_shared<string>(info.name);
  lock_guard<mutex> guard(threadListMutex);
  auto threadInfo = std::make_shared<threadInfo_t>(threadInfo_t{name, currentThread});
  threads->push_back(threadInfo);
}

shared_ptr<list<shared_ptr<threadInfo_t>>> JVM::getThreads() {
  return threads;
}

shared_ptr<threadInfo_t> JVM::findThread(const pthread_t &thread) {
  list<shared_ptr<threadInfo_t>>::iterator threadInfo;
  for (threadInfo = threads->begin(); threadInfo != threads->end(); threadInfo++) {
    if (pthread_equal((*threadInfo)->id, thread)) {
      return *threadInfo;
    }
  }
  return nullptr;
}

std::shared_ptr<jvmtiEnv> JVM::getJvmTi() {
  return jvmti;
}

JNIEnv *JVM::getJNIEnv() {
  JNIEnv *env;
  vm->GetEnv((void **) &env, JNI_VERSION_10);
  return env;
}

void JVM::attachThread() {
  JNIEnv *pEnv = getJNIEnv();
  vm->AttachCurrentThreadAsDaemon((void **) &pEnv, nullptr);
}

void JVM::dettachThread() {
  vm->DetachCurrentThread();
}

void JVM::setInstrumentationMetadata(InstrumentationMetadata metadata) {
  this->instrumentationMetadata = metadata;
}

jbyteArray JVM::instrumentCoroutines(const unsigned char *byteCode, int size) {
  JNIEnv *jni = this->getJNIEnv();
  jbyteArray jarray = jni->NewByteArray(size);
  jni->SetByteArrayRegion(jarray, 0, size, (jbyte *) byteCode);
  return (jbyteArray) jni->CallStaticObjectMethod(instrumentationMetadata.klass,
                                                  instrumentationMetadata.instrumentCoroutinesMethod,
                                                  jarray);
}

jbyteArray JVM::instrumentMethod(const unsigned char *byteCode, int size, shared_ptr<string> methodName) {
  JNIEnv *jni = this->getJNIEnv();
  jbyteArray jarray = jni->NewByteArray(size);
  auto jMethodName = jni->NewStringUTF(methodName->c_str());
  jni->SetByteArrayRegion(jarray, 0, size, (jbyte *) byteCode);
  return (jbyteArray) jni->CallStaticObjectMethod(instrumentationMetadata.klass,
                                                  instrumentationMetadata.instrumentMethod,
                                                  jarray,
                                                  jMethodName);
}
