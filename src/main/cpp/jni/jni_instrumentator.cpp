#include <pthread.h>

#include "../trace/coroutineTrace.hpp"
#include "jni_instrumentator.h"
#include "../utils/log.h"
#include "agent.hpp"

using namespace std;

extern "C" {

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineCreated(JNIEnv *env, jclass clazz,
                                                      jlong coroutineId) {
  pthread_t current_thread = pthread_self();
  auto thread_info = kotlinTracer::agent->getJVM()->findThread(current_thread);
  kotlinTracer::agent->getProfiler()->coroutineCreated(coroutineId);
  logDebug("coroutineCreated tid: " + *thread_info->name +
      " cid: " + to_string(coroutineId) +
      " from parentId: " + to_string(currentCoroutineId) + '\n');
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineResumed(JNIEnv *env, jclass clazz,
                                                      jlong coroutineId) {
  pthread_t current_thread = pthread_self();
  auto thread_info = kotlinTracer::agent->getJVM()->findThread(current_thread);
  kotlinTracer::agent->getProfiler()->coroutineResumed(coroutineId);
  logDebug("coroutine resumed tid: " + *thread_info->name +
      ", cid: " + to_string(coroutineId) + '\n');
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineCompleted(JNIEnv *env,
                                                        jclass clazz,
                                                        jlong coroutineId) {
  kotlinTracer::agent->getProfiler()->coroutineCompleted(coroutineId);
  logDebug("coroutineCompleted " + to_string(coroutineId) + '\n');
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_coroutineSuspend(JNIEnv *env, jclass clazz,
                                                      jlong coroutineId) {
  kotlinTracer::agent->getProfiler()->coroutineSuspended(coroutineId);
  logDebug("coroutineSuspend " + to_string(coroutineId) + '\n');
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_traceStart(JNIEnv *, jclass clazz) {
  kotlinTracer::agent->getProfiler()->traceStart(currentCoroutineId);
}

[[maybe_unused]] JNIEXPORT void JNICALL
Java_io_inst_CoroutineInstrumentator_traceEnd(JNIEnv *, jclass clazz, jlong t_coroutineId) {
  auto coroutineId = t_coroutineId == -2 ? currentCoroutineId : t_coroutineId;
  kotlinTracer::agent->getProfiler()->traceEnd(coroutineId);
}
}

