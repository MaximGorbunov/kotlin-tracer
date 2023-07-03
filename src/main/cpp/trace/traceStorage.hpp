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

namespace kotlin_tracer {
class TraceStorage {
 public:
  TraceStorage();

  void addRawTrace(TraceTime t_time, std::shared_ptr<ASGCTCallTrace> trace,
                   pthread_t thread, long long coroutine_id);
  void addProcessedTrace(const std::shared_ptr<ProcessedTraceRecord> &record);
  bool addOngoingTraceInfo(const TraceInfo &trace_info);
  void addSuspensionInfo(const std::shared_ptr<SuspensionInfo> &suspension_info);
  std::shared_ptr<SuspensionInfo> getLastSuspensionInfo(jlong coroutine_id);
  std::shared_ptr<kotlin_tracer::ConcurrentList<std::shared_ptr<SuspensionInfo>>> getSuspensions(jlong coroutine_id);
  void removeOngoingTraceInfo(const jlong &coroutine_id);
  TraceInfo &findOngoingTraceInfo(const jlong &coroutine_id);
  std::shared_ptr<RawCallTraceRecord> removeRawTraceHeader();
  std::shared_ptr<ConcurrentList<jlong>> getChildCoroutines(jlong coroutine_id);
  void addChildCoroutine(jlong coroutine_id, jlong parent_coroutine_id);
  void createChildCoroutineStorage(jlong coroutine_id);
  bool containsChildCoroutineStorage(jlong coroutine_id);

 private:
  std::unique_ptr<ConcurrentList<std::shared_ptr<RawCallTraceRecord>>> raw_list_;
  std::unique_ptr<ConcurrentList<std::shared_ptr<ProcessedTraceRecord>>> processed_list_;
  std::unique_ptr<ConcurrentMap<jlong, TraceInfo>> ongoing_trace_info_map_;
  std::unique_ptr<ConcurrentMap<jlong, std::shared_ptr<ConcurrentList<jlong>>>> child_coroutines_map_;
  std::unique_ptr<ConcurrentMap<jlong, std::shared_ptr<kotlin_tracer::ConcurrentList<std::shared_ptr<SuspensionInfo>>>>>
      suspensions_info_map_;
};
}

#endif // KOTLIN_TRACER_TRACESTORAGE_H
