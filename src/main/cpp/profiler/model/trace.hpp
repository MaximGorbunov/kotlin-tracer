#ifndef KOTLIN_TRACER_TRACE_H
#define KOTLIN_TRACER_TRACE_H

#include "jni_md.h"
#include "trace/traceTime.hpp"
#include "asyncTrace.hpp"
#include "../../concurrentCollections/concurrentList.h"

#include <memory>
#include <list>
#include <string>

namespace kotlin_tracer {
typedef struct RawCallTraceRecord {
  TraceTime time;
  int trace_count;
  std::shared_ptr<ASGCTCallTrace> trace;
  pthread_t thread;
  long long coroutine_id;
} RawCallTraceRecord;

typedef struct ProcessedTraceRecord {
  TraceTime time;
  int trace_count;
  std::shared_ptr<std::list<std::shared_ptr<std::string>>> method_info;
  std::shared_ptr<std::string> thread_name;
} ProcessedTraceRecord;

typedef struct TraceInfo {
  jlong coroutine_id;
  TraceTime start;
  TraceTime end;
} TraceInfo;

typedef struct SuspensionInfo {
  jlong coroutine_id;
  TraceTime start;
  TraceTime end;
  std::unique_ptr<std::list<std::shared_ptr<std::string>>> suspension_stack_trace;
} SuspensionInfo;

typedef struct ProcessedTraceInfo {
  jlong coroutine_id{};
  TraceTime trace_time{};
  TraceTime end{};
  ConcurrentList<SuspensionInfo> suspensions{};
} ProcessedTraceInfo;
}
#endif //KOTLIN_TRACER_TRACE_H
