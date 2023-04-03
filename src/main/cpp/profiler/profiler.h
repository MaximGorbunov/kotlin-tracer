#ifndef KOTLIN_TRACER_PROFILER_H
#define KOTLIN_TRACER_PROFILER_H

#include <csignal>
#include <unordered_map>

#include "asyncTrace.h"
#include "traceStorage.h"

class Profiler {
private:
  AsyncGetCallTrace async_trace_ptr;
  TraceStorage storage;
  std::unordered_map<jmethodID, std::shared_ptr<std::string>> methodInfoMap;
  bool active;

  Profiler();

  static std::shared_ptr<Profiler> instance;

public:
  void start_profiler();
  void stop();
  bool traceStart(const TraceInfo &trace_info);
  void traceEnd(const TraceInfo &trace_info);
  TraceInfo &findOngoingTrace(const jlong &coroutineId);
  TraceInfo &findCompletedTrace(const jlong &coroutineId);
  void removeOngoingTrace(const jlong &coroutineId);
  static std::shared_ptr<Profiler> getInstance();

private:
  void signal_action(int signo, siginfo_t *siginfo, void *ucontext);
  void processTraces();
  std::shared_ptr<std::string> processMethodInfo(jmethodID methodId,
                                                 jint lineno);
};

extern std::shared_ptr<Profiler> profiler;

#endif // KOTLIN_TRACER_PROFILER_H
