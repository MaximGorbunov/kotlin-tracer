#ifndef KOTLIN_TRACER_TRACE_H
#define KOTLIN_TRACER_TRACE_H

#include "jni_md.h"
#include "trace/traceTime.hpp"
#include "asyncTrace.hpp"

#include <memory>
#include <list>
#include <string>

namespace kotlinTracer {
typedef struct RawCallTraceRecord {
  TraceTime time;
  int trace_count;
  std::shared_ptr<ASGCTCallTrace> trace;
  pthread_t thread;
  long long coroutineId;
} RawCallTraceRecord;

typedef struct ProcessedTraceRecord {
  TraceTime time;
  int trace_count;
  std::shared_ptr<std::list<std::shared_ptr<std::string>>> methodInfo;
  std::shared_ptr<std::string> thread_name;
} ProcessedTraceRecord;

typedef struct TraceInfo {
  jlong coroutineId;
  TraceTime start;
  TraceTime end;
} TraceInfo;

typedef struct SuspensionInfo {
  jlong coroutineId;
  TraceTime start;
  TraceTime end;
  std::unique_ptr<std::list<std::shared_ptr<std::string>>> suspensionStackTrace;
} SuspensionInfo;

typedef struct ProcessedTraceInfo {
  jlong coroutineId{};
  TraceTime start{};
  TraceTime end{};
  ConcurrentList<SuspensionInfo> suspensions{};
} ProcessedTraceInfo;
}
#endif //KOTLIN_TRACER_TRACE_H
