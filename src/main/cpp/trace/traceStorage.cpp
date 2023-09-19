#include "traceStorage.hpp"

#include <sys/resource.h>
#include <memory>
#include <utility>
#include <thread>

#include "../utils/log.h"

#ifdef __APPLE__
#define RUSAGE_KIND RUSAGE_SELF
#elif defined(__linux__)
#define RUSAGE_KIND RUSAGE_THREAD
#endif

namespace kotlin_tracer {
using std::shared_ptr, std::make_shared, std::unique_ptr, std::make_unique, std::list;

TraceStorage::TraceStorage(
) : raw_list_(std::make_unique<ConcurrentList<shared_ptr<RawCallTraceRecord>>>()),
    processed_map_(std::make_unique<TraceMap>()),
    ongoing_trace_info_map_(std::make_unique<ConcurrentCleanableMap<jlong, TraceInfo>>()),
    child_coroutines_map_(std::make_unique<ConcurrentCleanableMap<jlong, std::shared_ptr<ConcurrentList<jlong>>>>()),
    coroutine_info_map_(std::make_unique<ConcurrentCleanableMap<jlong, shared_ptr<CoroutineInfo>>>()),
    gc_events_(std::make_unique<ConcurrentList<shared_ptr<GCEvent>>>()),
    active_(true) {
  cleaner_thread_ = make_unique<std::thread>([this]() {
    std::chrono::seconds sleep_time{15};
    while (active_.test(std::memory_order_relaxed)) {
      logDebug("Marking for clean");
      mark_for_clean();
      std::this_thread::sleep_for(sleep_time);
      logDebug("Cleaning storage");
      clean();
    }
  });
  cleaner_thread_->detach();
}

TraceStorage::~TraceStorage() {
  logDebug("Cleaning trace storage");
  active_.clear(std::memory_order_relaxed);
  cleaner_thread_.reset();
  logDebug("Cleaning trace storage finished");
}

void TraceStorage::addRawTrace(TraceTime time, const shared_ptr<AsyncTrace>& trace,
                               pthread_t thread, int64_t coroutine_id) {
  auto record = std::make_shared<RawCallTraceRecord>();
  record->time.set(time.get());
  record->trace = trace;
  record->thread = thread;
  record->trace_count = record->trace->size;
  record->coroutine_id = coroutine_id;
  raw_list_->push_back(record);
}

shared_ptr<RawCallTraceRecord> TraceStorage::removeRawTraceHeader() {
  return raw_list_->pop_front();
}

void TraceStorage::addProcessedTrace(jlong coroutine_id, const shared_ptr<ProcessedTraceRecord> &record) {
  if (processed_map_->contains(coroutine_id)) {
    processed_map_->at(coroutine_id)->push_back(record);
  } else {
    auto list = make_shared<ConcurrentList<std::shared_ptr<ProcessedTraceRecord>>>();
    list->push_back(record);
    processed_map_->insert(coroutine_id, list);
  }
}

bool TraceStorage::addOngoingTraceInfo(const TraceInfo &trace_info) {
  return ongoing_trace_info_map_->insert(trace_info.coroutine_id, trace_info);
}

TraceInfo &TraceStorage::findOngoingTraceInfo(const jlong &coroutine_id) {
  return ongoing_trace_info_map_->get(coroutine_id);
}

void TraceStorage::removeOngoingTraceInfo(const jlong &coroutine_id) {
  ongoing_trace_info_map_->erase(coroutine_id);
}

void TraceStorage::addSuspensionInfo(const shared_ptr<SuspensionInfo> &suspension_info) {
  auto coroutine_info = coroutine_info_map_->at(suspension_info->coroutine_id);
  coroutine_info->suspensions_list->push_back(suspension_info);
}

void TraceStorage::createCoroutineInfo(jlong coroutine_id) {
  auto coroutine_info = std::make_shared<CoroutineInfo>(
      currentTimeNs(),
      0,
      0,
      0,
      0,
      0,
      rusage{},
      std::make_shared<ConcurrentList<shared_ptr<SuspensionInfo>>>()
  );
  getrusage(RUSAGE_KIND, &coroutine_info->last_rusage);
  coroutine_info_map_->insert(
      coroutine_id,
      coroutine_info
);
}

shared_ptr<TraceStorage::CoroutineInfo> TraceStorage::getCoroutineInfo(jlong coroutine_id) const {
  if (coroutine_info_map_->contains(coroutine_id)) {
    return coroutine_info_map_->get(coroutine_id);
  } else {
    return {nullptr};
  }
}

shared_ptr<SuspensionInfo> TraceStorage::getLastSuspensionInfo(jlong coroutine_id) {
  std::function<bool(shared_ptr<SuspensionInfo>)>
      firstNonEndedSuspensionPredicate = [](const shared_ptr<SuspensionInfo> &suspensionInfo) {
    return suspensionInfo->end == 0;
  };
  if (coroutine_info_map_->contains(coroutine_id)) {
    return coroutine_info_map_->get(coroutine_id)->suspensions_list->find(firstNonEndedSuspensionPredicate);
  } else {
    return nullptr;
  }
}

shared_ptr<ConcurrentList<jlong>> TraceStorage::getChildCoroutines(jlong coroutine_id) const {
  return child_coroutines_map_->get(coroutine_id);
}

void TraceStorage::addChildCoroutine(jlong coroutine_id, jlong parent_coroutine_id) {
  child_coroutines_map_->get(parent_coroutine_id)->push_back(coroutine_id);
}

void TraceStorage::createChildCoroutineStorage(jlong coroutine_id) {
  child_coroutines_map_->insert(coroutine_id, std::make_shared<ConcurrentList<jlong>>());
}

bool TraceStorage::containsChildCoroutineStorage(jlong coroutine_id) const {
  return child_coroutines_map_->contains(coroutine_id);
}

TraceStorage::Traces TraceStorage::getProcessedTraces(jlong coroutine_id) const {
  if (processed_map_->contains(coroutine_id)) {
    return processed_map_->at(coroutine_id);
  } else {
    return {nullptr};
  }
}

void TraceStorage::mark_for_clean() {
  processed_map_->mark_current_values_for_clean();
  child_coroutines_map_->mark_current_values_for_clean();
  coroutine_info_map_->mark_current_values_for_clean();
  gc_events_->mark_for_clean();
}

void TraceStorage::clean() {
  processed_map_->clean();
  child_coroutines_map_->clean();
  coroutine_info_map_->clean();
  gc_events_->clean();
}

void TraceStorage::gcStart() {
  gc_events_->push_back(make_shared<GCEvent>(GCEvent{currentTimeNs(), TraceTime{0}}));
}

void TraceStorage::gcFinish() {
  auto gc_event = gc_events_->back();
  gc_events_->pop_front();
  gc_event->end = currentTimeNs();
  gc_events_->push_back(gc_event);
}

void TraceStorage::findGcEvents(
    const TraceTime &start,
    const TraceTime &stop,
    const std::function<void(shared_ptr<GCEvent>)>& for_each) const {
  std::function<bool(shared_ptr<GCEvent>)> filter = [start, stop](const shared_ptr<GCEvent>& event) {
    return event->start >= start && event->end <= stop;
  };
  gc_events_->forEachFiltered(filter, for_each);
}
}  // namespace kotlin_tracer
