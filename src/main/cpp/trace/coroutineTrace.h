#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_TRACE_COROUTINETRACE_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_TRACE_COROUTINETRACE_H_

#define NOT_FOUND -1

extern thread_local long currentCoroutineId;

void coroutineSuspended(long t_coroutineId, long t_spanId);
void coroutineResumed(long t_coroutineId, long t_spanId);

#endif //KOTLIN_TRACER_SRC_MAIN_CPP_TRACE_COROUTINETRACE_H_
