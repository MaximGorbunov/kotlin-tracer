#include "traceStorage.hpp"

#include <memory>

using namespace kotlin_tracer;
using std::shared_ptr, std::unique_ptr, std::list, std::make_unique, std::make_shared;

TraceStorage::TraceStorage()
    : m_rawList(make_unique<ConcurrentList<shared_ptr<RawCallTraceRecord>>>()),
      m_processedList(make_unique<ConcurrentList<shared_ptr<ProcessedTraceRecord>>>()),
      m_ongoingTraceInfoMap(make_unique<ConcurrentMap<jlong, TraceInfo>>()),
      m_childCoroutinesMap(make_unique<ConcurrentMap<jlong, std::shared_ptr<ConcurrentList<jlong>>>>()),
      m_suspensionsInfoMap(make_unique<ConcurrentMap<jlong,
                                                     shared_ptr<ConcurrentList<shared_ptr<SuspensionInfo>>>>>()) {}

void TraceStorage::addRawTrace(TraceTime t_time, shared_ptr<ASGCTCallTrace> t_trace,
                               pthread_t t_thread, long long t_coroutineId) {
  auto record = make_shared<RawCallTraceRecord>(RawCallTraceRecord{});
  record->time = t_time;
  record->trace = std::move(t_trace);
  record->thread = t_thread;
  record->trace_count = record->trace->num_frames;
  record->coroutine_id = t_coroutineId;
  m_rawList->push_back(record);
}

shared_ptr<RawCallTraceRecord> TraceStorage::removeRawTraceHeader() {
  return m_rawList->pop_front();
}

void TraceStorage::addProcessedTrace(const shared_ptr<ProcessedTraceRecord> &t_record) {
  m_processedList->push_back(t_record);
}

bool TraceStorage::addOngoingTraceInfo(const TraceInfo &t_traceInfo) {
  return m_ongoingTraceInfoMap->insert(t_traceInfo.coroutine_id, t_traceInfo);
}

TraceInfo &TraceStorage::findOngoingTraceInfo(const jlong &t_coroutineId) {
  return m_ongoingTraceInfoMap->get(t_coroutineId);
}

void TraceStorage::removeOngoingTraceInfo(const jlong &t_coroutineId) {
  m_ongoingTraceInfoMap->erase(t_coroutineId);
}

void TraceStorage::addSuspensionInfo(const shared_ptr<SuspensionInfo> &t_suspensionInfo) {
  auto creationLambda = []() { return std::make_shared<ConcurrentList<shared_ptr<SuspensionInfo>>>(); };
  auto list = m_suspensionsInfoMap->findOrInsert(t_suspensionInfo->coroutine_id, creationLambda);
  list->push_back(t_suspensionInfo);
}

shared_ptr<ConcurrentList<shared_ptr<SuspensionInfo>>> TraceStorage::getSuspensions(jlong t_coroutineId) {
  if (m_suspensionsInfoMap->contains(t_coroutineId)) {
    return m_suspensionsInfoMap->get(t_coroutineId);
  } else return {nullptr};
}

shared_ptr<SuspensionInfo> TraceStorage::getLastSuspensionInfo(jlong t_coroutineId) {
  std::function<bool(shared_ptr<SuspensionInfo>)>
      firstNonEndedSuspensionPredicate = [](const shared_ptr<SuspensionInfo> &suspensionInfo) {
    return suspensionInfo->end == 0;
  };
  if (m_suspensionsInfoMap->contains(t_coroutineId)) {
    return m_suspensionsInfoMap->get(t_coroutineId)->find(firstNonEndedSuspensionPredicate);
  } else return nullptr;
}

shared_ptr<ConcurrentList<jlong>> TraceStorage::getChildCoroutines(jlong t_coroutineId) {
  return m_childCoroutinesMap->get(t_coroutineId);
}

void TraceStorage::addChildCoroutine(jlong t_coroutineId, jlong t_parentCoroutineId) {
  m_childCoroutinesMap->get(t_parentCoroutineId)->push_back(t_coroutineId);
}

void TraceStorage::createChildCoroutineStorage(jlong t_coroutineId) {
  m_childCoroutinesMap->insert(t_coroutineId, std::make_shared<ConcurrentList<jlong>>());
}

bool TraceStorage::containsChildCoroutineStorage(jlong t_coroutineId) {
  return m_childCoroutinesMap->contains(t_coroutineId);
}
