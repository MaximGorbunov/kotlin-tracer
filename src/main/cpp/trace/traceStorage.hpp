#ifndef KOTLIN_TRACER_TRACESTORAGE_H
#define KOTLIN_TRACER_TRACESTORAGE_H

#include <list>
#include <memory>
#include <string>
#include <jni.h>

#include "concurrentCollections/concurrentList.h"
#include "concurrentCollections/concurrentMap.h"
#include "../profiler/model/trace.hpp"

namespace kotlin_tracer {
class TraceStorage {
 public:
  TraceStorage();

  struct CoroutineInfo {
    TraceTime last_resume;
    TraceTime running_time{0};
    std::shared_ptr<ConcurrentList<std::shared_ptr<SuspensionInfo>>> suspensions_list;
  };

  typedef std::shared_ptr<ConcurrentList<std::shared_ptr<ProcessedTraceRecord>>> Traces;
  typedef ConcurrentMap<jlong, Traces> TraceMap;

  void addRawTrace(TraceTime t_time, std::shared_ptr<ASGCTCallTrace> trace,
                   pthread_t thread, int64_t coroutine_id);
  void addProcessedTrace(jlong coroutine_id, const std::shared_ptr<ProcessedTraceRecord> &record);
  [[nodiscard]] Traces getProcessedTraces(jlong coroutine_id) const;
  bool addOngoingTraceInfo(const TraceInfo &trace_info);
  void addSuspensionInfo(const std::shared_ptr<SuspensionInfo> &suspension_info);
  std::shared_ptr<SuspensionInfo> getLastSuspensionInfo(jlong coroutine_id);
  [[nodiscard]] std::shared_ptr<CoroutineInfo> getCoroutineInfo(jlong coroutine_id) const;
  void removeOngoingTraceInfo(const jlong &coroutine_id);
  TraceInfo &findOngoingTraceInfo(const jlong &coroutine_id);
  std::shared_ptr<RawCallTraceRecord> removeRawTraceHeader();
  [[nodiscard]] std::shared_ptr<ConcurrentList<jlong>> getChildCoroutines(jlong coroutine_id) const;
  void addChildCoroutine(jlong coroutine_id, jlong parent_coroutine_id);
  void createChildCoroutineStorage(jlong coroutine_id);
  [[nodiscard]] bool containsChildCoroutineStorage(jlong coroutine_id) const;
  void createCoroutineInfo(jlong coroutine_id);

 private:
  std::unique_ptr<ConcurrentList<std::shared_ptr<RawCallTraceRecord>>> raw_list_;
  std::unique_ptr<TraceMap> processed_map_;
  std::unique_ptr<ConcurrentMap<jlong, TraceInfo>> ongoing_trace_info_map_;
  std::unique_ptr<ConcurrentMap<jlong, std::shared_ptr<ConcurrentList<jlong>>>> child_coroutines_map_;
  std::unique_ptr<ConcurrentMap<jlong, std::shared_ptr<CoroutineInfo>>> coroutine_info_map_;
};
}

#endif // KOTLIN_TRACER_TRACESTORAGE_H
