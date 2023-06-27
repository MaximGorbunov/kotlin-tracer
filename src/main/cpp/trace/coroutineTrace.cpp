#include "coroutineTrace.hpp"

thread_local long currentCoroutineId = NOT_FOUND;

void kotlinTracer::coroutineSuspended(long t_coroutineId) {
  currentCoroutineId = NOT_FOUND;
}

void kotlinTracer::coroutineResumed(long t_coroutineId) {
  currentCoroutineId = t_coroutineId;
}

void kotlinTracer::coroutineCompleted() {
  currentCoroutineId = NOT_FOUND;
}