#include <memory>
#include <cstring>

#include "utils/argsParser.hpp"
#include "utils/jarLoader.hpp"
#include "utils/log.h"
#include "vm/jvm.hpp"
#include "agent.hpp"

using std::function, std::shared_mutex, std::shared_ptr, std::unique_ptr, std::make_unique, std::mutex, std::lock_guard, std::string, std::to_string, std::runtime_error;

typedef std::shared_lock<std::shared_mutex> read_lock;
typedef std::unique_lock<std::shared_mutex> write_lock;

static unique_ptr<kotlin_tracer::Agent> agent;
static shared_mutex mutex;

// Required to be enabled for AsyncTrace usage
void JNICALL ClassLoad(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv, ::jthread t_thread, jclass t_class) {}

void JNICALL ClassFileLoadHook(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv,
                               jclass t_classBeingRedefined, jobject t_loader,
                               const char *t_name, jobject t_protectionDomain,
                               jint t_classDataLen,
                               const unsigned char *t_classData,
                               jint *t_newClassDataLen,
                               unsigned char **t_newClassData) {
  agent->getInstrumentation()->instrument(t_jniEnv,
                                          t_name,
                                          t_classDataLen,
                                          t_classData,
                                          t_newClassDataLen,
                                          t_newClassData);
}

void JNICALL ThreadStart(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv, ::jthread t_thread) {
  agent->getJVM()->addCurrentThread(t_thread);
}

void JNICALL VMInit(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv, ::jthread t_thread) {
  agent->getJVM()->initializeMethodIds(t_jvmtiEnv, t_jniEnv);
  auto metadata = load(agent->getInstrumentation()->getJarPath(),
                       "kotlin-tracer.jar",
                       agent->getJVM());
  agent->getInstrumentation()->setInstrumentationMetadata(std::move(metadata));
}

void JNICALL ClassPrepare(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv, ::jthread t_thread,
                          jclass t_klass) {
  agent->getJVM()->loadMethodsId(t_jvmtiEnv, t_jniEnv, t_klass);
}

void JNICALL VMStart(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv) {
  agent->getProfiler()->startProfiler();
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *t_vm, char *t_options, void *t_reserved) {
  if (t_options == nullptr || strlen(t_options) == 0) {
    throw runtime_error("method name required for interception!");
  }
  string optionsStr(t_options);
  logDebug("======Kotlin tracer initialization start======");
  logDebug("Options:" + optionsStr);
  auto profilerOptions = kotlin_tracer::ArgsParser::parseProfilerOptions(optionsStr);
  logDebug("Parsed class:" + *profilerOptions->class_name);
  logDebug("Parsed method:" + *profilerOptions->method_name);
  logDebug("Parsed period:" + to_string(profilerOptions->profiling_period.count()));

  auto callbacks = make_unique<jvmtiEventCallbacks>();
  callbacks->VMStart = VMStart;
  callbacks->ThreadStart = ThreadStart;
  callbacks->ClassPrepare = ClassPrepare;
  callbacks->ClassLoad = ClassLoad;
  callbacks->ClassFileLoadHook = ClassFileLoadHook;
  callbacks->VMInit = VMInit;
  agent = make_unique<kotlin_tracer::Agent>(std::shared_ptr<JavaVM>(t_vm, [](JavaVM *vm) {}),
                                            std::move(callbacks),
                                            std::move(profilerOptions));
  return 0;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *t_vm) {
  write_lock lock(mutex);
  agent.reset();
  logDebug("End of cleanup");
}

namespace kotlin_tracer {
void coroutineCreated(jlong coroutine_id) {
  read_lock lock(mutex);
  if (agent != nullptr) {
    agent->getProfiler()->coroutineCreated(coroutine_id);
  }
}

void coroutineResumed(jlong coroutine_id) {
  read_lock lock(mutex);
  if (agent != nullptr) {
    agent->getProfiler()->coroutineResumed(coroutine_id);
  }
}

void coroutineSuspended(jlong coroutine_id) {
  read_lock lock(mutex);
  if (agent != nullptr) {
    agent->getProfiler()->coroutineSuspended(coroutine_id);
  }
}

void coroutineCompleted(jlong coroutine_id) {
  read_lock lock(mutex);
  if (agent != nullptr) {
    agent->getProfiler()->coroutineCompleted(coroutine_id);
  }
}

void traceStart() {
  read_lock lock(mutex);
  if (agent != nullptr) {
    agent->getProfiler()->traceStart();
  }
}

void traceEnd(jlong coroutine_id) {
  read_lock lock(mutex);
  if (agent != nullptr) {
    agent->getProfiler()->traceEnd(coroutine_id);
  }
}

shared_ptr<JVM> Agent::getJVM() {
  return jvm_;
}

shared_ptr<Instrumentation> Agent::getInstrumentation() {
  return instrumentation_;
}

shared_ptr<Profiler> Agent::getProfiler() {
  return profiler_;
}
}