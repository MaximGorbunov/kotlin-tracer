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
  void addSuspensionInfo(const std::shared_ptr<SuspensionInfo>& t_suspensionInfo);
  std::shared_ptr<SuspensionInfo> getLastSuspensionInfo(jlong t_coroutineId);
  std::shared_ptr<kotlinTracer::ConcurrentList<std::shared_ptr<SuspensionInfo>>> getSuspensions(jlong t_coroutineId);
  void removeOngoingTraceInfo(const jlong &t_coroutineId);
  TraceInfo &findOngoingTraceInfo(const jlong &t_coroutineId);
  std::shared_ptr<RawCallTraceRecord> removeRawTraceHeader();
  std::shared_ptr<ConcurrentList<jlong>> getChildCoroutines(jlong t_coroutineId);
  void addChildCoroutine(jlong t_coroutineId, jlong t_parentCoroutineId);
  void createChildCoroutineStorage(jlong t_coroutineId);
  bool containsChildCoroutineStorage(jlong t_coroutineId);

 private:
  std::unique_ptr<ConcurrentList<std::shared_ptr<RawCallTraceRecord>>> m_rawList;
  std::unique_ptr<ConcurrentList<std::shared_ptr<ProcessedTraceRecord>>> m_processedList;
  std::unique_ptr<ConcurrentMap<jlong, TraceInfo>> m_ongoingTraceInfoMap;
  std::unique_ptr<ConcurrentMap<jlong, std::shared_ptr<ConcurrentList<jlong>>>> m_childCoroutinesMap;
  std::unique_ptr<ConcurrentMap<jlong, std::shared_ptr<kotlinTracer::ConcurrentList<std::shared_ptr<SuspensionInfo>>>>> m_suspensionsInfoMap;
};
}

#endif // KOTLIN_TRACER_TRACESTORAGE_H
