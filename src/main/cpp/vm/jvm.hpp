#ifndef KOTLIN_TRACER_JVM_H
#define KOTLIN_TRACER_JVM_H

#include <jvmti.h>
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <list>
#include <pthread.h>

#include "concurrentCollections/concurrentList.h"

namespace kotlinTracer {
typedef struct threadInfo {
  const std::shared_ptr<std::string> name;
  const pthread_t id;
} ThreadInfo;

typedef struct InstrumentationMetadata {
  jclass klass;
  jmethodID instrumentCoroutinesMethod;
  jmethodID instrumentMethod;
} InstrumentationMetadata;

class JVM {
 public:
  JVM(std::shared_ptr<JavaVM> t_vm, jvmtiEventCallbacks *t_callbacks);
  ~JVM();
  void addCurrentThread(jthread t_thread);
  std::shared_ptr<ConcurrentList<std::shared_ptr<ThreadInfo>>> getThreads();
  std::shared_ptr<ThreadInfo> findThread(const pthread_t &t_thread);
  jvmtiEnv* getJvmTi();
  JNIEnv *getJNIEnv();
  void attachThread();
  void dettachThread();
  void initializeMethodIds(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv);
  void loadMethodsId(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv, jclass t_class);

 private:
  std::shared_ptr<JavaVM> m_vm;
  jvmtiEnv* m_jvmti;
  std::shared_ptr<ConcurrentList<std::shared_ptr<ThreadInfo>>> m_threads;
};
}
#endif //KOTLIN_TRACER_JVM_H
