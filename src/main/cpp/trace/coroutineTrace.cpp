#include "coroutineTrace.hpp"

thread_local long currentCoroutineId = NOT_FOUND;

void kotlin_tracer::coroutineSuspended(long t_coroutineId) {
  currentCoroutineId = NOT_FOUND;
}

void kotlin_tracer::coroutineResumed(long t_coroutineId) {
  currentCoroutineId = t_coroutineId;
}

void kotlin_tracer::coroutineCompleted() {
  currentCoroutineId = NOT_FOUND;
}