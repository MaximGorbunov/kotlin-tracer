#include "traceStorage.hpp"

#include <memory>

using namespace kotlinTracer;
using std::shared_ptr, std::unique_ptr, std::list, std::make_unique, std::make_shared, std::move;

TraceStorage::TraceStorage()
    : m_rawList(make_unique<ConcurrentList<shared_ptr<RawCallTraceRecord>>>()),
      m_processedList(make_unique<ConcurrentList<shared_ptr<ProcessedTraceRecord>>>()),
      m_ongoingTraceInfoMap(make_unique<ConcurrentMap<jlong, TraceInfo>>()),
      m_suspensionsInfoMap(make_unique<ConcurrentMap<jlong, shared_ptr<ConcurrentList<shared_ptr<SuspensionInfo>>>>>()) {}

void TraceStorage::addRawTrace(TraceTime t_time, shared_ptr<ASGCTCallTrace> t_trace,
                               pthread_t t_thread, long long t_coroutineId) {
  auto record = make_shared<RawCallTraceRecord>(RawCallTraceRecord{});
  record->time = t_time;
  record->trace = move(t_trace);
  record->thread = t_thread;
  record->trace_count = record->trace->numFrames;
  record->coroutineId = t_coroutineId;
  m_rawList->push_back(record);
}

shared_ptr<RawCallTraceRecord> TraceStorage::removeRawTraceHeader() {
  return m_rawList->pop_front();
}

void TraceStorage::addProcessedTrace(const shared_ptr<ProcessedTraceRecord> &t_record) {
  m_processedList->push_back(t_record);
}

bool TraceStorage::addOngoingTraceInfo(const TraceInfo &t_traceInfo) {
  return m_ongoingTraceInfoMap->insert(t_traceInfo.coroutineId, t_traceInfo);
}

TraceInfo &TraceStorage::findOngoingTraceInfo(const jlong &t_coroutineId) {
  return m_ongoingTraceInfoMap->get(t_coroutineId);
}

void TraceStorage::removeOngoingTraceInfo(const jlong &t_coroutineId) {
  m_ongoingTraceInfoMap->erase(t_coroutineId);
}

void TraceStorage::addSuspensionInfo(const shared_ptr<SuspensionInfo> &t_suspensionInfo) {
  auto creationLambda = []() { return shared_ptr<ConcurrentList<shared_ptr<SuspensionInfo>>>(new ConcurrentList<shared_ptr<SuspensionInfo>>()); };
  auto list = m_suspensionsInfoMap->findOrInsert(t_suspensionInfo->coroutineId, creationLambda);
  list->push_back(t_suspensionInfo);
}

shared_ptr<ConcurrentList<shared_ptr<SuspensionInfo>>> TraceStorage::getSuspensions(jlong t_coroutineId) {
  if (m_suspensionsInfoMap->contains(t_coroutineId)) {
    return m_suspensionsInfoMap->get(t_coroutineId);
  } else return {nullptr};
}

shared_ptr<SuspensionInfo> TraceStorage::getLastSuspensionInfo(jlong t_coroutineId) {
  if (m_suspensionsInfoMap->contains(t_coroutineId)) {
    return m_suspensionsInfoMap->get(t_coroutineId)->back();
  } else return nullptr;
}
