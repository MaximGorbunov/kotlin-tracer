#ifndef KOTLIN_TRACER_JVM_H
#define KOTLIN_TRACER_JVM_H

#include <jvmti.h>
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <list>
#include <pthread.h>

namespace kotlinTracer {
typedef struct threadInfo {
  std::shared_ptr<std::string> name;
  pthread_t id;
} ThreadInfo;

typedef struct InstrumentationMetadata {
  jclass klass;
  jmethodID instrumentCoroutinesMethod;
  jmethodID instrumentMethod;
} InstrumentationMetadata;

class JVM {
 public:
  JVM(JavaVM *t_vm, jvmtiEventCallbacks *t_callbacks);
  ~JVM();
  void addCurrentThread(jthread t_thread);
  std::shared_ptr<std::list<std::shared_ptr<ThreadInfo>>> getThreads();
  std::shared_ptr<ThreadInfo> findThread(const pthread_t &t_thread);
  std::shared_ptr<jvmtiEnv> getJvmTi();
  JNIEnv *getJNIEnv();
  void attachThread();
  void dettachThread();
  void initializeMethodIds(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv);
  void loadMethodsId(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv, jclass t_class);

 private:
  JavaVM *m_vm;
  std::shared_ptr<jvmtiEnv> m_jvmti;
  std::mutex m_threadListMutex;
  std::shared_ptr<std::list<std::shared_ptr<ThreadInfo>>> m_threads;
};
}
#endif //KOTLIN_TRACER_JVM_H
