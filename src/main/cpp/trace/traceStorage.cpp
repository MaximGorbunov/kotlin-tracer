#include "traceStorage.hpp"

#include <memory>
#include <utility>

namespace kotlin_tracer {
using std::shared_ptr, std::unique_ptr, std::list;

TraceStorage::TraceStorage(
) : raw_list_(std::make_unique<ConcurrentList<shared_ptr<RawCallTraceRecord>>>()),
    processed_list_(std::make_unique<ConcurrentList<shared_ptr<ProcessedTraceRecord>>>()),
    ongoing_trace_info_map_(std::make_unique<ConcurrentMap<jlong, TraceInfo>>()),
    child_coroutines_map_(std::make_unique<ConcurrentMap<jlong,
                                                         std::shared_ptr<ConcurrentList<jlong>>>>()),
    suspensions_info_map_(std::make_unique<ConcurrentMap<jlong,
                                                         shared_ptr<ConcurrentList<shared_ptr<
                                                             SuspensionInfo>>>>>()) {
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

void TraceStorage::addProcessedTrace(const shared_ptr<ProcessedTraceRecord> &record) {
  processed_list_->push_back(record);
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
  auto creationLambda = []() { return std::make_shared<ConcurrentList<shared_ptr<SuspensionInfo>>>(); };
  auto list = suspensions_info_map_->findOrInsert(suspension_info->coroutine_id, creationLambda);
  list->push_back(suspension_info);
}

shared_ptr<ConcurrentList<shared_ptr<SuspensionInfo>>> TraceStorage::getSuspensions(jlong coroutine_id) const {
  if (suspensions_info_map_->contains(coroutine_id)) {
    return suspensions_info_map_->get(coroutine_id);
  } else {
    return {nullptr};
  }
}

shared_ptr<SuspensionInfo> TraceStorage::getLastSuspensionInfo(jlong coroutine_id) {
  std::function<bool(shared_ptr<SuspensionInfo>)>
      firstNonEndedSuspensionPredicate = [](const shared_ptr<SuspensionInfo> &suspensionInfo) {
    return suspensionInfo->end == 0;
  };
  if (suspensions_info_map_->contains(coroutine_id)) {
    return suspensions_info_map_->get(coroutine_id)->find(firstNonEndedSuspensionPredicate);
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
}  // namespace kotlin_tracer
