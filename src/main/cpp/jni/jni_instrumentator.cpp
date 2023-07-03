#include <pthread.h>

#include "jni_instrumentator.h"
#include "agent.hpp"
namespace kotlin_tracer {
extern "C" {
[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineCreated(JNIEnv *env, jclass clazz,
                                                      jlong coroutine_id) {
  coroutineCreated(coroutine_id);
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineResumed(JNIEnv *env, jclass clazz,
                                                      jlong coroutine_id) {
  coroutineResumed(coroutine_id);
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineCompleted(JNIEnv *env,
                                                        jclass clazz,
                                                        jlong coroutine_id) {
  coroutineCompleted(coroutine_id);
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineSuspend(JNIEnv *env, jclass clazz,
                                                      jlong coroutine_id) {
  coroutineSuspended(coroutine_id);
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_traceStart(JNIEnv *, jclass clazz) {
  traceStart();
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_traceEnd(JNIEnv *, jclass clazz, jlong coroutine_id) {
  traceEnd(coroutine_id);
}
}
}
