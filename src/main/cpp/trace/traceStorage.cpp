#include "traceStorage.hpp"

#include <memory>

using namespace std;

kotlinTracer::TraceStorage::TraceStorage() {
  m_rawList = make_unique<ConcurrentList<shared_ptr<RawCallTraceRecord>>>();
  m_processedList = make_unique<ConcurrentList<shared_ptr<ProcessedTraceRecord>>>();
  m_ongoingTraceInfoMap = make_unique<ConcurrentMap<jlong, TraceInfo>>();
  m_completedTraceInfoMap = make_unique<ConcurrentMap<jlong, TraceInfo>>();
}

void kotlinTracer::TraceStorage::addRawTrace(kotlinTracer::TraceTime t_time, std::shared_ptr<ASGCTCallTrace> t_trace,
                                             pthread_t t_thread, long long t_coroutineId) {
  auto record = shared_ptr<RawCallTraceRecord>((RawCallTraceRecord *) calloc(1, sizeof(RawCallTraceRecord)));
  record->time = t_time;
  record->trace = std::move(t_trace);
  record->thread = t_thread;
  record->trace_count = record->trace->numFrames;
  record->coroutineId = t_coroutineId;
  m_rawList->push_back(record);
}

shared_ptr<kotlinTracer::RawCallTraceRecord> kotlinTracer::TraceStorage::removeRawTraceHeader() {
  return m_rawList->pop_front();
}

void kotlinTracer::TraceStorage::addProcessedTrace(const shared_ptr<ProcessedTraceRecord> &t_record) {
  m_processedList->push_back(t_record);
}

bool kotlinTracer::TraceStorage::addOngoingTraceInfo(const TraceInfo &t_traceInfo) {
  return m_ongoingTraceInfoMap->insert(t_traceInfo.coroutineId, t_traceInfo);
}

kotlinTracer::TraceInfo &kotlinTracer::TraceStorage::findOngoingTraceInfo(const jlong &t_coroutineId) {
  return m_ongoingTraceInfoMap->get(t_coroutineId);
}

void kotlinTracer::TraceStorage::addCompletedTraceInfo(const TraceInfo &t_traceInfo) {
  m_completedTraceInfoMap->insert(t_traceInfo.coroutineId, t_traceInfo);
}

kotlinTracer::TraceInfo &kotlinTracer::TraceStorage::findCompletedTraceInfo(const jlong &t_coroutineId) {
  return m_completedTraceInfoMap->get(t_coroutineId);
}

void kotlinTracer::TraceStorage::removeOngoingTraceInfo(const jlong &t_coroutineId) {
  m_ongoingTraceInfoMap->erase(t_coroutineId);
}

void kotlinTracer::TraceStorage::addSuspensionInfo(std::shared_ptr<kotlinTracer::SuspensionInfo> t_suspensionInfo) {
  auto creationLambda = []() { return make_shared<list<shared_ptr<SuspensionInfo>>>(); };
  auto list = m_suspensionsInfoMap->findOrInsert(t_suspensionInfo->coroutineId, creationLambda);
  list->push_back(t_suspensionInfo);
}

shared_ptr<kotlinTracer::SuspensionInfo> kotlinTracer::TraceStorage::getSuspensionInfo(jlong t_coroutineId) {
  if (m_suspensionsInfoMap->contains(t_coroutineId)) {
    return m_suspensionsInfoMap->get(t_coroutineId)->back();
  } else return nullptr;
}
