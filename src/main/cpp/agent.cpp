#include <memory>
#include <cstring>

#include "utils/argsParser.hpp"
#include "utils/jarLoader.hpp"
#include "utils/log.h"
#include "vm/jvm.hpp"
#include "agent.hpp"

using namespace std;
// Required to be enabled for AsyncTrace usage

unique_ptr<kotlin_tracer::Agent> kotlin_tracer::agent;
std::mutex kotlin_tracer::agentMutex = std::mutex();

void JNICALL ClassLoad(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv, ::jthread t_thread, jclass t_class) {}

void JNICALL ClassFileLoadHook(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv,
                               jclass t_classBeingRedefined, jobject t_loader,
                               const char *t_name, jobject t_protectionDomain,
                               jint t_classDataLen,
                               const unsigned char *t_classData,
                               jint *t_newClassDataLen,
                               unsigned char **t_newClassData) {
  kotlin_tracer::agent->getInstrumentation()->instrument(t_jniEnv,
                                                        t_name,
                                                        t_classDataLen,
                                                        t_classData,
                                                        t_newClassDataLen,
                                                        t_newClassData);
}

void JNICALL ThreadStart(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv, ::jthread t_thread) {
  kotlin_tracer::agent->getJVM()->addCurrentThread(t_thread);
}

void JNICALL VMInit(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv, ::jthread t_thread) {
  kotlin_tracer::agent->getJVM()->initializeMethodIds(t_jvmtiEnv, t_jniEnv);
  auto metadata = kotlin_tracer::JarLoader::load(kotlin_tracer::agent->getInstrumentation()->getJarPath(),
                                                "kotlin-tracer.jar",
                                                kotlin_tracer::agent->getJVM());
  kotlin_tracer::agent->getInstrumentation()->setInstrumentationMetadata(std::move(metadata));
}

void JNICALL ClassPrepare(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv, ::jthread t_thread,
                          jclass t_klass) {
  kotlin_tracer::agent->getJVM()->loadMethodsId(t_jvmtiEnv, t_jniEnv, t_klass);
}

void JNICALL VMStart(jvmtiEnv *t_jvmtiEnv, JNIEnv *t_jniEnv) {
  kotlin_tracer::agent->getProfiler()->startProfiler();
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
  kotlin_tracer::agent = make_unique<kotlin_tracer::Agent>(std::shared_ptr<JavaVM>(t_vm, [](JavaVM* vm){}), std::move(callbacks), std::move(profilerOptions));
  return 0;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *t_vm) {
  lock_guard guard(kotlin_tracer::agentMutex);
  kotlin_tracer::agent.reset(nullptr);
  logDebug("End of cleanup");
}

shared_ptr<kotlin_tracer::JVM> kotlin_tracer::Agent::getJVM() {
  return m_jvm;
}

shared_ptr<kotlin_tracer::Instrumentation> kotlin_tracer::Agent::getInstrumentation() {
  return m_instrumentation;
}

shared_ptr<kotlin_tracer::Profiler> kotlin_tracer::Agent::getProfiler() {
  return m_profiler;
}

void kotlin_tracer::Agent::stop() {
  m_profiler->stop();
}
