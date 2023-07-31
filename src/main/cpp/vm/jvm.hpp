#ifndef KOTLIN_TRACER_JVM_H
#define KOTLIN_TRACER_JVM_H

#include <jvmti.h>
#include <memory>
#include <unordered_map>
#include <thread>
#include <list>
#include <utility>
#include <pthread.h>

#include "concurrentCollections/concurrentList.h"
#include "vmStructs.h"
#include "jvmCodeCache.h"

namespace kotlin_tracer {

typedef struct threadInfo {
  const std::shared_ptr<std::string> name;
  const pthread_t id;
  std::atomic_int current_traces = 0;
  threadInfo(std::shared_ptr<std::string> _name, pthread_t _id): name(std::move(_name)), id(_id), current_traces(0) {}
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
  bool isJavaFrame(uint64_t address);
  jmethodID getJMethodId(uint64_t address, uint64_t frame_pointer);
  const addr2Symbol::function_info *getNativeFunctionInfo(intptr_t address);

 private:
  std::shared_ptr<JavaVM> java_vm_;
  jvmtiEnv *jvmti_env_;
  std::unique_ptr<JVMCodeCache> jvm_code_cache_;
  std::shared_ptr<ConcurrentList<std::shared_ptr<ThreadInfo>>> threads_;
  std::unordered_map<std::string, std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Field>>>> vm_structs_;
  std::unordered_map<std::string, VMTypeEntry> types_;
  addr2Symbol::Addr2Symbol addr_2_symbol_;
  void resolveVMStructs();
  void resolveVMTypes();
};
}
#endif //KOTLIN_TRACER_JVM_H
