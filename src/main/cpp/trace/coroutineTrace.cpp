#include "coroutineTrace.h"

thread_local long currentCoroutineId = NOT_FOUND;

void coroutineSuspended(long t_coroutineId, long t_spanId) {
  currentCoroutineId = NOT_FOUND;
}

void coroutineResumed(long t_coroutineId, long t_spanId) {
  currentCoroutineId = t_coroutineId;
}