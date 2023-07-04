#ifndef KOTLIN_TRACER_JVM_H
#define KOTLIN_TRACER_JVM_H

#include <jvmti.h>
#include <memory>
#include <unordered_map>
#include <thread>
#include <list>
#include <pthread.h>

#include "concurrentCollections/concurrentList.h"

namespace kotlin_tracer {

typedef struct threadInfo {
  const std::shared_ptr<std::string> name;
  const pthread_t id;
} ThreadInfo;

typedef struct InstrumentationMetadata {
  jclass klass;
  jmethodID instrument_coroutines_method;
  jmethodID instrument_method;
} InstrumentationMetadata;

class JVM {
 public:
  JVM(std::shared_ptr<JavaVM> java_vm, jvmtiEventCallbacks *callbacks);
  void addCurrentThread(jthread thread);
  std::shared_ptr<ConcurrentList<std::shared_ptr<ThreadInfo>>> getThreads();
  std::shared_ptr<ThreadInfo> findThread(const pthread_t &thread);
  jvmtiEnv *getJvmTi();
  JNIEnv *getJNIEnv();
  void attachThread();
  void dettachThread();
  static void initializeMethodIds(jvmtiEnv *jvmti_env, JNIEnv *jni_env);
  static void loadMethodsId(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jclass klass);

 private:
  std::shared_ptr<JavaVM> java_vm_;
  jvmtiEnv *jvmti_env_;
  std::shared_ptr<ConcurrentList<std::shared_ptr<ThreadInfo>>> threads_;
};
}
#endif //KOTLIN_TRACER_JVM_H
