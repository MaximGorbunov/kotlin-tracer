#ifndef KOTLIN_TRACER_TRACESTORAGE_H
#define KOTLIN_TRACER_TRACESTORAGE_H

#include <list>
#include <memory>
#include <string>
#include <jni.h>
#include <thread>
#include <utility>
#include <sys/resource.h>

#include "concurrentCollections/concurrentList.h"
#include "concurrentCollections/concurrentMap.h"
#include "../profiler/model/trace.hpp"

namespace kotlin_tracer {
class TraceStorage {
 public:
  TraceStorage();
  ~TraceStorage();

  struct CoroutineInfo {
    TraceTime last_resume;
    TraceTime wall_clock_running_time{0};
    TraceTime cpu_user_clock_running_time_us{0};
    TraceTime cpu_system_clock_running_time_us{0};
    uint64_t voluntary_switches{0};
    uint64_t involuntary_switches{0};
    rusage last_rusage;
    std::shared_ptr<ConcurrentList<std::shared_ptr<SuspensionInfo>>> suspensions_list;
    CoroutineInfo(
        const TraceTime &a_last_resume,
        long long a_wall_clock_running_time,
        long long a_cpu_user_clock_running_time_us,
        long long a_cpu_system_clock_running_time_us,
        uint64_t a_voluntary_switches,
        uint64_t a_involuntary_switches,
        rusage a_last_rusage,
        std::shared_ptr<ConcurrentList<std::shared_ptr<SuspensionInfo>>> a_suspensions_list
    ) : last_resume(a_last_resume),
        wall_clock_running_time(a_wall_clock_running_time),
        cpu_user_clock_running_time_us(a_cpu_user_clock_running_time_us),
        cpu_system_clock_running_time_us(a_cpu_system_clock_running_time_us),
        voluntary_switches(a_voluntary_switches),
        involuntary_switches(a_involuntary_switches),
        last_rusage(a_last_rusage),
        suspensions_list(std::move(a_suspensions_list)) {}
  };

  struct GCEvent {
    TraceTime start;
    TraceTime end;
  };

  typedef std::shared_ptr<ConcurrentList<std::shared_ptr<ProcessedTraceRecord>>> Traces;
  typedef ConcurrentCleanableMap<jlong, Traces> TraceMap;

  void addRawTrace(TraceTime t_time, const std::shared_ptr<AsyncTrace> &trace,
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
  void gcStart();
  void gcFinish();
  void findGcEvents(const TraceTime &start,
                    const TraceTime &stop,
                    const std::function<void(std::shared_ptr<GCEvent>)> &for_each) const;
 private:
  std::unique_ptr<ConcurrentList<std::shared_ptr<RawCallTraceRecord>>> raw_list_;
  std::unique_ptr<TraceMap> processed_map_;
  std::unique_ptr<ConcurrentCleanableMap<jlong, TraceInfo>> ongoing_trace_info_map_;
  std::unique_ptr<ConcurrentCleanableMap<jlong, std::shared_ptr<ConcurrentList<jlong>>>> child_coroutines_map_;
  std::unique_ptr<ConcurrentCleanableMap<jlong, std::shared_ptr<CoroutineInfo>>> coroutine_info_map_;
  std::unique_ptr<ConcurrentList<std::shared_ptr<GCEvent>>> gc_events_;
  std::atomic_flag active_;
  std::unique_ptr<std::thread> cleaner_thread_;

  void mark_for_clean();
  void clean();
};
}

#endif // KOTLIN_TRACER_TRACESTORAGE_H
