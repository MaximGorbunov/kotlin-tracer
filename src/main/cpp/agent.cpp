#include <memory>

#include "utils/argsParser.hpp"
#include "utils/jarLoader.hpp"
#include "utils/log.h"
#include "vm/jvm.hpp"
#include "agent.hpp"

using namespace std;
// Required to be enabled for AsyncTrace usage

unique_ptr<kotlinTracer::Agent> kotlinTracer::agent;
void JNICALL ClassLoad(jvmtiEnv *jvmtiEnv, JNIEnv *jniEnv, jthread thread, jclass klass) {}

void JNICALL ClassFileLoadHook(jvmtiEnv *jvmtiEnv, JNIEnv *jniEnv,
                               jclass classBeingRedefined, jobject loader,
                               const char *name, jobject protectionDomain,
                               jint classDataLen,
                               const unsigned char *classData,
                               jint *newClassDataLen,
                               unsigned char **newClassData) {
  kotlinTracer::agent->getInstrumentation()->instrument(jniEnv,
                                                        name,
                                                        classDataLen,
                                                        classData,
                                                        newClassDataLen,
                                                        newClassData);
}

void JNICALL ThreadStart(jvmtiEnv *jvmtiEnv, JNIEnv *jniEnv, jthread thread) {
  kotlinTracer::agent->getJVM()->addCurrentThread(thread);
}

void JNICALL VMInit(jvmtiEnv *jvmtiEnv, JNIEnv *jniEnv, jthread thread) {
  kotlinTracer::agent->getJVM()->initializeMethodIds(jvmtiEnv, jniEnv);
  auto metadata = kotlinTracer::JarLoader::load("kotlin-tracer.jar", kotlinTracer::agent->getJVM());
  kotlinTracer::agent->getInstrumentation()->setInstrumentationMetadata(std::move(metadata));
}

void JNICALL ClassPrepare(jvmtiEnv *jvmtiEnv, JNIEnv *jniEnv, jthread thread,
                          jclass klass) {
  kotlinTracer::agent->getJVM()->loadMethodsId(jvmtiEnv, jniEnv, klass);
}

void JNICALL VMStart(jvmtiEnv *jvmtiEnv, JNIEnv *jniEnv) {
  kotlinTracer::agent->getProfiler()->startProfiler();
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
  if (options == nullptr || strlen(options) == 0) {
    throw runtime_error("method name required for interception!");
  }
  string optionsStr(options);
  logDebug("======Kotlin tracer initialization start======");
  logDebug("Options:" + optionsStr);
  auto profilerOptions = kotlinTracer::ArgsParser::parseProfilerOptions(optionsStr);
  logDebug("Parsed class:" + *profilerOptions->className);
  logDebug("Parsed method:" + *profilerOptions->methodName);
  logDebug("Parsed period:" + to_string(profilerOptions->profilingPeriod.count()));

  auto callbacks = make_unique<jvmtiEventCallbacks>();
  callbacks->VMStart = VMStart;
  callbacks->ThreadStart = ThreadStart;
  callbacks->ClassPrepare = ClassPrepare;
  callbacks->ClassLoad = ClassLoad;
  callbacks->ClassFileLoadHook = ClassFileLoadHook;
  callbacks->VMInit = VMInit;
  kotlinTracer::agent = make_unique<kotlinTracer::Agent>(vm, std::move(callbacks), std::move(profilerOptions));
  return 0;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm) {
  kotlinTracer::agent.reset();
}

shared_ptr<kotlinTracer::JVM> kotlinTracer::Agent::getJVM() {
  return m_jvm;
}

shared_ptr<kotlinTracer::Instrumentation> kotlinTracer::Agent::getInstrumentation() {
  return m_instrumentation;
}

shared_ptr<kotlinTracer::Profiler> kotlinTracer::Agent::getProfiler() {
  return m_profiler;
}
