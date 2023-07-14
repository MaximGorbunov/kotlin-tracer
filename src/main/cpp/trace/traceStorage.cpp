#include "traceStorage.hpp"

#include <memory>
#include <utility>
#include <thread>

namespace kotlin_tracer {
using std::shared_ptr, std::make_shared, std::unique_ptr, std::list;

TraceStorage::TraceStorage(
) : raw_list_(std::make_unique<ConcurrentList<shared_ptr<RawCallTraceRecord>>>()),
    processed_map_(std::make_unique<TraceMap>()),
    ongoing_trace_info_map_(std::make_unique<ConcurrentCleanableMap<jlong, TraceInfo>>()),
    child_coroutines_map_(std::make_unique<ConcurrentCleanableMap<jlong, std::shared_ptr<ConcurrentList<jlong>>>>()),
    coroutine_info_map_(std::make_unique<ConcurrentCleanableMap<jlong, shared_ptr<CoroutineInfo>>>()),
    active_(true) {
  cleaner_thread_ = std::thread([this]() {
    std::chrono::seconds sleep_time{15};
    while (active_.test(std::memory_order_relaxed)) {
      mark_for_clean();
      std::this_thread::sleep_for(sleep_time);
      clean();
    }
  });
}

TraceStorage::~TraceStorage() {
  active_.clear(std::memory_order_relaxed);
  cleaner_thread_.join();
}

void TraceStorage::addRawTrace(TraceTime time, shared_ptr<ASGCTCallTrace> trace,
                               pthread_t thread, int64_t coroutine_id) {
  auto record = std::make_shared<RawCallTraceRecord>(RawCallTraceRecord{});
  record->time = time;
  record->trace = std::move(trace);
  record->thread = thread;
  record->trace_count = record->trace->num_frames;
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
  coroutine_info_map_->insert(
      coroutine_id,
      std::make_shared<CoroutineInfo>(CoroutineInfo{
          0,
          0,
          std::make_shared<ConcurrentList<shared_ptr<SuspensionInfo>>>()
      }));
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
}

void TraceStorage::clean() {
  processed_map_->clean();
  child_coroutines_map_->clean();
  coroutine_info_map_->clean();
}
}  // namespace kotlin_tracer
