#include <atomic>
#include <chrono>
#include <iostream>
#include <pthread.h>
#include <memory>
#include <string>

#include "jni_instrumentator.h"
#include "jvm.h"
#include "log.h"
#include "profiler.h"
#include "traceTime.h"

using namespace std;

extern "C" {

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineCreated(JNIEnv *env, jclass clazz,
                                                      jlong coroutineId,
                                                      jlong parentId) {
  pthread_t current_thread = pthread_self();
  auto thread_info = JVM::getInstance()->findThread(current_thread);
  log_debug("coroutineCreated tid: " + *thread_info->name +
            " cid: " + to_string(coroutineId) +
            " from parentId: " + to_string(parentId) + '\n');
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineResumed(JNIEnv *env, jclass clazz,
                                                      jlong coroutineId) {
  pthread_t current_thread = pthread_self();
  auto thread_info = JVM::getInstance()->findThread(current_thread);
  log_debug("coroutine resumed tid: " + *thread_info->name +
            ", cid: " + to_string(coroutineId) + '\n');
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineCompleted(JNIEnv *env,
                                                        jclass clazz,
                                                        jlong coroutineId) {
  log_debug("coroutineCompleted " + to_string(coroutineId) + '\n');
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineSuspend(JNIEnv *env, jclass clazz,
                                                      jlong coroutineId) {
  log_debug("coroutineSuspend " + to_string(coroutineId) + '\n');
}

[[maybe_unused]] JNIEXPORT jlong JNICALL
Java_io_inst_CoroutineInstrumentator_traceStart(JNIEnv *, jclass clazz,
                                                jlong coroutineId) {
  auto charBuffer = unique_ptr<char>(new char[100]);
  pthread_getname_np(pthread_self(), charBuffer.get(), 100);
  auto c = std::make_unique<string>(charBuffer.get());
  auto start = current_time_ns();
  TraceInfo trace_info{coroutineId, start, 0};
  if (profiler->traceStart(trace_info)) {
    log_debug("trace start: " + to_string(start) + ":" + *c);
  } else {
        throw runtime_error("Found trace that already started");
  }
  return coroutineId;
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_traceEnd(JNIEnv *, jclass clazz,
                                              jlong coroutineId, jlong spanId) {
  auto finish = current_time_ns();
  auto traceInfo = profiler->findOngoingTrace(coroutineId);
  traceInfo.end = finish;
  profiler->traceEnd(traceInfo);
  log_debug("trace end: " + to_string(finish) + ":" + to_string(spanId));
  log_debug(to_string(traceInfo.end - traceInfo.start));
}
}

