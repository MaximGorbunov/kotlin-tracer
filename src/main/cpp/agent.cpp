#include <memory>
#include <thread>

#include "argsParser.h"
#include "jarLoader.h"
#include "jvm.h"
#include "log.h"
#include "profiler.h"

using namespace std;

shared_ptr<JVM> jvm;
shared_ptr<Profiler> profiler;
shared_ptr<profilerOptions> optionsPtr;
unique_ptr<thread> profilerThread;

// Required to be enabled for AsyncTrace usage
void JNICALL ClassLoad(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread,
                       jclass klass) {}

void JNICALL
ClassFileLoadHook(__attribute__((unused)) jvmtiEnv *jvmti_env, JNIEnv *jni_env,
                  __attribute__((unused)) jclass class_being_redefined,
                  __attribute__((unused)) jobject loader, const char *name,
                  __attribute__((unused)) jobject protection_domain,
                  jint class_data_len, const unsigned char *class_data,
                  jint *new_class_data_len, unsigned char **new_class_data) {
  if (!strcmp(optionsPtr->className->c_str(), name)) {
    log_debug("Found tracing class!" + *optionsPtr->className);
    jbyteArray transformed = JVM::getInstance()->instrumentMethod(
        class_data, class_data_len, optionsPtr->methodName);
    jboolean copy = false;
    auto *new_code =
        (unsigned char *)jni_env->GetByteArrayElements(transformed, &copy);
    *new_class_data_len = jni_env->GetArrayLength(transformed);
    *new_class_data = new_code;
  }
  if (!strcmp("kotlinx/coroutines/debug/internal/DebugProbesImpl", name)) {
    jbyteArray transformed =
        JVM::getInstance()->instrumentCoroutines(class_data, class_data_len);
    jboolean copy = false;
    auto *new_code =
        (unsigned char *)jni_env->GetByteArrayElements(transformed, &copy);
    *new_class_data_len = jni_env->GetArrayLength(transformed);
    *new_class_data = new_code;
  }
}

void JNICALL ThreadStart(jvmtiEnv *jvmti_env,
                         __attribute__((unused)) JNIEnv *jni_env,
                         jthread thread) {
  JVM::getInstance()->addCurrentThread(thread);
}

// The jmethodIDs should be allocated. Or we'll get 0 method id
void loadMethodsId(jvmtiEnv *jvmti_env, __attribute__((unused)) JNIEnv *jni,
                   jclass klass) {
  jint method_count = 0;
  jmethodID *methods = nullptr;
  if (jvmti_env->GetClassMethods(klass, &method_count, &methods) == 0) {
    jvmti_env->Deallocate((unsigned char *)methods);
  }
}

void JNICALL VMInit(jvmtiEnv *jvmti_env, JNIEnv *jni_env,
                    __attribute__((unused)) jthread thread) {
  jint counter = 0;
  jclass *classes;
  jvmtiError err = jvmti_env->GetLoadedClasses(&counter, &classes);
  if (err != JVMTI_ERROR_NONE) {
    printf("Failed to load classes\n");
    fflush(stdout);
  }
  for (int i = 0; i < counter; i++) {
    jclass klass = (classes)[i];
    loadMethodsId(jvmti_env, jni_env, klass);
  }
  jvmti_env->Deallocate((unsigned char *)classes);
  JarLoader::load("kotlin-tracer.jar");
}

void JNICALL ClassPrepare(jvmtiEnv *jvmti_env, JNIEnv *jni_env,
                          __attribute__((unused)) jthread thread,
                          jclass klass) {
  loadMethodsId(jvmti_env, jni_env, klass);
}

void JNICALL VMStart(__attribute__((unused)) jvmtiEnv *jvmti_env,
                     __attribute__((unused)) JNIEnv *jni_env) {
  profilerThread = std::make_unique<thread>([] { profiler->start_profiler(); });
}

__attribute__((unused)) JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *vm, __attribute__((unused)) char *options,
             __attribute__((unused)) void *reserved) {
  if (options == nullptr || strlen(options) == 0) {
    throw runtime_error("method name required for interception!");
  }
  shared_ptr<string> optionsStr = make_shared<string>(options);
  log_debug("======Kotlin tracer initialization start======");
  log_debug("Options:" + *optionsStr);
  optionsPtr = ArgsParser::parseProfilerOptions(optionsStr);
  log_debug("Parsed class:" + *optionsPtr->className);
  log_debug("Parsed method:" + *optionsPtr->methodName);
  log_debug("Parsed period:" + to_string(optionsPtr->profilingPeriod.count()));

  jvmtiEventCallbacks callbacks = {nullptr};
  callbacks.VMStart = VMStart;
  callbacks.ThreadStart = ThreadStart;
  callbacks.ClassPrepare = ClassPrepare;
  callbacks.ClassLoad = ClassLoad;
  callbacks.ClassFileLoadHook = ClassFileLoadHook;
  callbacks.VMInit = VMInit;
  jvm = shared_ptr<JVM>(JVM::createInstance(vm, &callbacks));
  log_debug("[Kotlin-tracer] JVM instance created");
  profiler = shared_ptr<Profiler>(Profiler::getInstance());
  log_debug("[Kotlin-tracer] Profiler instance created");
  return 0;
}

__attribute__((unused)) JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm) {
  log_debug("[Kotlin-tracer] Unloading agent");
  profiler->stop();
  profilerThread->join();
  jvm.reset();
  profiler.reset();
  optionsPtr.reset();
}
