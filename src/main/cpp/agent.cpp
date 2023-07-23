#include <memory>
#include <cstring>
#include <string>
#include <utility>

#include "utils/argsParser.hpp"
#include "utils/jarLoader.hpp"
#include "utils/log.h"
#include "vm/jvm.hpp"
#include "agent.hpp"

using std::function, std::shared_mutex, std::shared_ptr, std::unique_ptr,
    std::make_unique, std::lock_guard, std::string,
    std::to_string, std::runtime_error;

typedef std::shared_lock<std::shared_mutex> read_lock;
typedef std::unique_lock<std::shared_mutex> write_lock;

static unique_ptr<kotlin_tracer::Agent> agent;
static shared_mutex mutex;
static std::atomic_flag attached{false};

// Required to be enabled for AsyncTrace usage
void JNICALL ClassLoad(__attribute__((unused)) jvmtiEnv *jvmti_env,
                       __attribute__((unused)) JNIEnv *jni_env,
                       __attribute__((unused)) ::jthread thread,
                       __attribute__((unused)) jclass klass) {}

void JNICALL ClassFileLoadHook(
    __attribute__((unused))jvmtiEnv *jvmti_env,
    JNIEnv *jni_env,
    __attribute__((unused))jclass class_being_redefined,
    __attribute__((unused))jobject loader,
    const char *name,
    __attribute__((unused))jobject protection_domain,
    jint class_data_len,
    const unsigned char *class_data,
    jint *new_class_data_len,
    unsigned char **new_class_data
) {
  agent->getInstrumentation()->instrument(jni_env,
                                          name,
                                          class_data_len,
                                          class_data,
                                          new_class_data_len,
                                          new_class_data);
}

void JNICALL ThreadStart(
    __attribute__((unused))jvmtiEnv *jvmti_env,
    __attribute__((unused))JNIEnv *jni_env,
    ::jthread thread) {
  agent->getJVM()->addCurrentThread(thread);
}

void JNICALL VMInit(
    jvmtiEnv *jvmti_env,
    JNIEnv *jni_env,
    __attribute__((unused)) ::jthread thread
) {
  agent->getJVM()->initializeMethodIds(jvmti_env, jni_env);
  auto metadata = load(agent->getInstrumentation()->getJarPath(),
                       "kotlin-tracer.jar",
                       agent->getJVM());
  agent->getInstrumentation()->setInstrumentationMetadata(std::move(metadata));
  //Install coroutine debug probes
  auto debug_probes = jni_env->FindClass("kotlinx/coroutines/debug/DebugProbes");
  auto install_method = jni_env->GetMethodID(debug_probes, "install", "()V");
  auto instance_field = jni_env->GetStaticFieldID(debug_probes, "INSTANCE", "Lkotlinx/coroutines/debug/DebugProbes;");
  auto instance = jni_env->GetStaticObjectField(debug_probes, instance_field);
  jni_env->CallVoidMethod(instance, install_method);

  agent->getProfiler()->startProfiler();
}

void JNICALL ClassPrepare(
    jvmtiEnv *jvmti_env,
    JNIEnv *jni_env,
    __attribute__((unused)) ::jthread thread,
    jclass t_klass
) {
  agent->getJVM()->loadMethodsId(jvmti_env, jni_env, t_klass);
}

void JNICALL VMStart(
    __attribute__((unused)) jvmtiEnv *jvmti_env,
    __attribute__((unused)) JNIEnv *jni_env
) {
}

[[maybe_unused]]
void JNICALL GarbageCollectionStart(__attribute__((unused)) jvmtiEnv *jvmti_env) {
    read_lock lock(mutex);
    if (agent != nullptr) {
      agent->getProfiler()->gcStart();
    }
}

[[maybe_unused]]
void JNICALL GarbageCollectionFinish(__attribute__((unused)) jvmtiEnv *jvmti_env) {
    read_lock lock(mutex);
    if (agent != nullptr) {
      agent->getProfiler()->gcFinish();
    }
}

[[maybe_unused]]
JNIEXPORT jint JNICALL Agent_OnLoad(
    JavaVM *java_vm,
    char *options,
    __attribute__((unused)) void *reserved
) {
  if (!attached.test_and_set(std::memory_order::relaxed)) { // Initialize once
    if (options == nullptr || strlen(options) == 0) {
      throw runtime_error("method name required for interception!");
    }
    string optionsStr(options);
    logDebug("======Kotlin tracer initialization start======");
    logDebug("Options:" + optionsStr);
    auto profilerOptions =
        kotlin_tracer::ArgsParser::parseProfilerOptions(optionsStr);
    logDebug("Parsed class:" + *profilerOptions->class_name);
    logDebug("Parsed method:" + *profilerOptions->method_name);
    logDebug(
        "Parsed period:" + to_string(profilerOptions->profiling_period.count()));

    auto callbacks = make_unique<jvmtiEventCallbacks>();
    callbacks->VMStart = VMStart;
    callbacks->ThreadStart = ThreadStart;
    callbacks->ClassPrepare = ClassPrepare;
    callbacks->ClassLoad = ClassLoad;
    callbacks->ClassFileLoadHook = ClassFileLoadHook;
    callbacks->VMInit = VMInit;
    callbacks->GarbageCollectionStart = GarbageCollectionStart;
    callbacks->GarbageCollectionFinish = GarbageCollectionFinish;
    agent = make_unique<kotlin_tracer::Agent>(
        std::shared_ptr<JavaVM>(java_vm,
                                [](JavaVM *vm) {}),
        std::move(callbacks),
        std::move(profilerOptions));
  }
  return 0;
}

[[maybe_unused]]
JNIEXPORT void JNICALL Agent_OnUnload(__attribute__((unused)) JavaVM *java_vm) {
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
}  // namespace kotlin_tracer
