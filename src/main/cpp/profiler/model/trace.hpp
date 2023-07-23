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
  int64_t coroutine_id;
} RawCallTraceRecord;

typedef struct StackFrame {
  std::shared_ptr<std::string> frame;
  jint line_number;
} StackFrame;

typedef struct ProcessedTraceRecord {
  std::shared_ptr<std::list<std::unique_ptr<StackFrame>>> stack_trace;
  std::shared_ptr<std::string> thread_name;
  TraceTime time;
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
  std::unique_ptr<std::list<std::unique_ptr<StackFrame>>> suspension_stack_trace;
} SuspensionInfo;

typedef struct ProcessedTraceInfo {
  jlong coroutine_id{};
  TraceTime trace_time{};
  TraceTime end{};
  ConcurrentList<SuspensionInfo> suspensions{};
} ProcessedTraceInfo;
}
#endif //KOTLIN_TRACER_TRACE_H
