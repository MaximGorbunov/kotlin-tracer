#ifndef KOTLIN_TRACER_TRACE_H
#define KOTLIN_TRACER_TRACE_H

#include "jni_md.h"
#include "traceTime.h"
#include "asyncTrace.h"

#include <memory>
#include <list>
#include <string>

struct RawCallTraceRecord {
  trace_time time;
  int trace_count;
  std::shared_ptr<ASGCT_CallTrace> trace;
  pthread_t thread;
};

struct ProcessedTraceRecord {
  trace_time time;
  int trace_count;
  std::shared_ptr<std::list<std::shared_ptr<std::string>>> methodInfo;
  std::shared_ptr<std::string> thread_name;
};

struct TraceInfo {
  jlong coroutineId;
  trace_time start;
  trace_time end;
};

#endif //KOTLIN_TRACER_TRACE_H
