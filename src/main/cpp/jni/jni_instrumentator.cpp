#include <thread>

#include "jni_instrumentator.h"
#include "agent.hpp"
namespace kotlin_tracer {

extern "C" {
[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineCreated(
    __attribute__((unused)) JNIEnv *env,
    __attribute__((unused)) jclass clazz,
    jlong coroutine_id) {
  if (coroutine_id == -1) {
    coroutine_id = static_cast<jlong>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
  }
  coroutineCreated(coroutine_id);
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineResumed(
    __attribute__((unused)) JNIEnv *env,
    __attribute__((unused)) jclass clazz,
    jlong coroutine_id) {
  if (coroutine_id == -1) {
    coroutine_id = static_cast<jlong>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
  }
  coroutineResumed(coroutine_id);
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineCompleted(
    __attribute__((unused)) JNIEnv *env,
    __attribute__((unused)) jclass clazz,
    jlong coroutine_id) {
  if (coroutine_id == -1) {
    coroutine_id = static_cast<jlong>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
  }
  coroutineCompleted(coroutine_id);
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineSuspend(
    __attribute__((unused)) JNIEnv *env,
    __attribute__((unused)) jclass clazz,
    jlong coroutine_id) {
  if (coroutine_id == -1) {
    coroutine_id = static_cast<jlong>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
  }
  coroutineSuspended(coroutine_id);
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_traceStart(
    JNIEnv *,
    __attribute__((unused)) jclass clazz, jlong coroutine_id) {
  traceStart(coroutine_id);
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_traceEnd(
    JNIEnv *,
    __attribute__((unused)) jclass clazz,
    jlong coroutine_id) {
  traceEnd(coroutine_id);
}
}

}  // namespace kotlin_tracer
