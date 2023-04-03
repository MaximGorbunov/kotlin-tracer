#ifndef KOTLIN_TRACER_TRACESTORAGE_H
#define KOTLIN_TRACER_TRACESTORAGE_H

#include <list>
#include <memory>
#include <mutex>
#include <string>

#include "concurrentList.h"
#include "concurrentMap.h"
#include "jni_md.h"
#include "trace.h"

class TraceStorage {
private:
  std::unique_ptr<ConcurrentList<std::shared_ptr<RawCallTraceRecord>>> rawList;
  std::unique_ptr<ConcurrentList<std::shared_ptr<ProcessedTraceRecord>>> processedList;
  std::unique_ptr<ConcurrentMap<jlong, TraceInfo>> ongoingTraceInfoMap;
  std::unique_ptr<ConcurrentMap<jlong, TraceInfo>> completedTraceInfoMap;

public:
  TraceStorage();

  void addRawTrace(trace_time time, std::shared_ptr<ASGCT_CallTrace> trace,
                   pthread_t thread);
  void addProcessedTrace(const std::shared_ptr<ProcessedTraceRecord> &record);
  bool addOngoingTraceInfo(const TraceInfo &trace_info);
  void addCompletedTraceInfo(const TraceInfo &trace_info);
  void removeOngoingTraceInfo(const jlong &coroutineId);
  void removeCompletedTraceInfo(const jlong &coroutineId);
  TraceInfo &findOngoingTraceInfo(const jlong &coroutineId);
  TraceInfo &findCompletedTraceInfo(const jlong &coroutineId);
  std::shared_ptr<RawCallTraceRecord> removeRawTraceHeader();
};

#endif // KOTLIN_TRACER_TRACESTORAGE_H
