#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_TRACE_COROUTINETRACE_HPP_
#define KOTLIN_TRACER_SRC_MAIN_CPP_TRACE_COROUTINETRACE_HPP_

#define NOT_FOUND (-1)

extern thread_local long currentCoroutineId;
namespace kotlinTracer {
void coroutineSuspended(long t_coroutineId);
void coroutineResumed(long t_coroutineId);
void coroutineCompleted();
}

#endif //KOTLIN_TRACER_SRC_MAIN_CPP_TRACE_COROUTINETRACE_HPP_
