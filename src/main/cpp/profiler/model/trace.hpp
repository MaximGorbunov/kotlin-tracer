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
  TraceTime time{0};
  int64_t trace_count;
  std::unique_ptr<AsyncTrace> trace;
  pthread_t thread;
  int64_t coroutine_id;
  RawCallTraceRecord(): time(0), trace_count(0), trace(nullptr), thread{}, coroutine_id(0) {};
} RawCallTraceRecord;

typedef struct StackFrame {
  std::string *frame;
  std::string *base;
} StackFrame;

typedef struct ProcessedTraceRecord {
  std::unique_ptr<std::list<std::unique_ptr<StackFrame>>> stack_trace;
  std::string* thread_name;
  TraceTime time{0};
} ProcessedTraceRecord;

typedef struct TraceInfo {
  jlong coroutine_id;
  TraceTime start;
  TraceTime end;
  TraceInfo(
      jlong a_coroutine_id,
      const TraceTime& a_start,
      const TraceTime& a_end
  ) : coroutine_id(a_coroutine_id),
      start(a_start),
      end(a_end) {}
  TraceInfo() : coroutine_id(0), start(0), end(0) {}
} TraceInfo;

typedef struct SuspensionInfo {
  jlong coroutine_id;
  TraceTime start;
  TraceTime end;
  std::unique_ptr<std::list<std::unique_ptr<StackFrame>>> suspension_stack_trace;
} SuspensionInfo;

typedef struct ProcessedTraceInfo {
  jlong coroutine_id{};
  TraceTime trace_time{0};
  TraceTime end{0};
  ConcurrentList<SuspensionInfo> suspensions{};
} ProcessedTraceInfo;
}
#endif //KOTLIN_TRACER_TRACE_H
