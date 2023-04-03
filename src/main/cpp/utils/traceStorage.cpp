#include "traceStorage.h"

#include <memory>

using namespace std;

TraceStorage::TraceStorage() {
  rawList = make_unique<ConcurrentList<shared_ptr<RawCallTraceRecord>>>();
  processedList = make_unique<ConcurrentList<shared_ptr<ProcessedTraceRecord>>>();
  ongoingTraceInfoMap = make_unique<ConcurrentMap<jlong, TraceInfo>>();
  completedTraceInfoMap = make_unique<ConcurrentMap<jlong, TraceInfo>>();
}

void TraceStorage::addRawTrace(trace_time time, shared_ptr<ASGCT_CallTrace> trace, pthread_t thread) {
  auto record = shared_ptr<RawCallTraceRecord>((RawCallTraceRecord *) calloc(1, sizeof(RawCallTraceRecord)));
  record->time = time;
  record->trace = std::move(trace);
  record->thread = thread;
  record->trace_count = record->trace->num_frames;
  rawList->push_back(record);
}

shared_ptr<RawCallTraceRecord> TraceStorage::removeRawTraceHeader() {
  return rawList->pop_front();
}

void TraceStorage::addProcessedTrace(const shared_ptr<ProcessedTraceRecord> &record) {
  processedList->push_back(record);
}

bool TraceStorage::addOngoingTraceInfo(const TraceInfo &trace_info) {
  return ongoingTraceInfoMap->insert(trace_info.coroutineId, trace_info);
}

TraceInfo& TraceStorage::findOngoingTraceInfo(const jlong &coroutineId) {
  return ongoingTraceInfoMap->find(coroutineId);
}

void TraceStorage::addCompletedTraceInfo(const TraceInfo &trace_info) {
  completedTraceInfoMap->insert(trace_info.coroutineId, trace_info);
}

TraceInfo& TraceStorage::findCompletedTraceInfo(const jlong &coroutineId) {
  return completedTraceInfoMap->find(coroutineId);
}

void TraceStorage::removeOngoingTraceInfo(const jlong &coroutineId) {
  ongoingTraceInfoMap->erase(coroutineId);
}
