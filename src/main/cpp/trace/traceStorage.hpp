#ifndef KOTLIN_TRACER_TRACESTORAGE_H
#define KOTLIN_TRACER_TRACESTORAGE_H

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <jni.h>

#include "concurrentCollections/concurrentList.h"
#include "concurrentCollections/concurrentMap.h"
#include "../profiler/model/trace.hpp"

namespace kotlinTracer {
class TraceStorage {
 public:
  TraceStorage();

  void addRawTrace(TraceTime t_time, std::shared_ptr<ASGCTCallTrace> t_trace,
                   pthread_t t_thread, long long t_coroutineId);
  void addProcessedTrace(const std::shared_ptr<ProcessedTraceRecord> &t_record);
  bool addOngoingTraceInfo(const TraceInfo &t_traceInfo);
  void addCompletedTraceInfo(const TraceInfo &t_traceInfo);
  void addSuspensionInfo(std::shared_ptr<SuspensionInfo> t_suspensionInfo);
  std::shared_ptr<SuspensionInfo> getSuspensionInfo(jlong t_coroutineId);
  void removeOngoingTraceInfo(const jlong &t_coroutineId);
  TraceInfo &findOngoingTraceInfo(const jlong &t_coroutineId);
  TraceInfo &findCompletedTraceInfo(const jlong &t_coroutineId);
  std::shared_ptr<RawCallTraceRecord> removeRawTraceHeader();

 private:
  std::unique_ptr<ConcurrentList<std::shared_ptr<RawCallTraceRecord>>> m_rawList;
  std::unique_ptr<ConcurrentList<std::shared_ptr<ProcessedTraceRecord>>> m_processedList;
  std::unique_ptr<ConcurrentMap<jlong, TraceInfo>> m_ongoingTraceInfoMap;
  std::unique_ptr<ConcurrentMap<jlong, TraceInfo>> m_completedTraceInfoMap;
  std::unique_ptr<ConcurrentMap<jlong, std::shared_ptr<std::list<std::shared_ptr<SuspensionInfo>>>>> m_suspensionsInfoMap;
};
}

#endif // KOTLIN_TRACER_TRACESTORAGE_H
