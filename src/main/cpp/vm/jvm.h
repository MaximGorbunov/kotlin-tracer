#ifndef KOTLIN_TRACER_JVM_H
#define KOTLIN_TRACER_JVM_H

#include <jvmti.h>
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <list>
#include <pthread.h>

typedef struct threadInfo {
  std::shared_ptr<std::string> name;
  pthread_t id;
} threadInfo_t;

typedef struct InstrumentationMetadata {
  jclass klass;
  jmethodID instrumentCoroutinesMethod;
  jmethodID instrumentMethod;
} InstrumentationMetadata;

class JVM {
 private:
  JavaVM *vm;
  std::shared_ptr<jvmtiEnv> jvmti;
  std::mutex threadListMutex;

  std::shared_ptr<std::list<std::shared_ptr<threadInfo_t>>> threads;

  JVM(JavaVM *vm, jvmtiEventCallbacks *callbacks);

  static std::shared_ptr<JVM> instance;

  InstrumentationMetadata instrumentationMetadata;
 public:
  ~JVM();

  static std::shared_ptr<JVM> createInstance(JavaVM *vm, jvmtiEventCallbacks *callbacks);

  static std::shared_ptr<JVM> getInstance();

  void addCurrentThread(jthread thread);

  std::shared_ptr<std::list<std::shared_ptr<threadInfo_t>>> getThreads();

  std::shared_ptr<threadInfo_t> findThread(const pthread_t &thread);

  std::shared_ptr<jvmtiEnv> getJvmTi();

  JNIEnv *getJNIEnv();

  void attachThread();
  void dettachThread();

  void setInstrumentationMetadata(InstrumentationMetadata metadata);
  jbyteArray instrumentCoroutines(const unsigned char *byteCode, int size);
  jbyteArray instrumentMethod(const unsigned char *byteCode, int size, std::shared_ptr<std::string> methodName);
};

#endif //KOTLIN_TRACER_JVM_H
