#ifndef KOTLIN_TRACER_PROFILER_H
#define KOTLIN_TRACER_PROFILER_H

#include <csignal>
#include <unordered_map>

#include "model/asyncTrace.hpp"
#include "trace/traceStorage.hpp"
#include "../vm/jvm.hpp"

#define NOT_FOUND (-1)

namespace kotlin_tracer {

class Profiler {
 public:
  static std::shared_ptr<Profiler> create(std::shared_ptr<JVM> t_jvm, std::chrono::nanoseconds t_threshold);
  static std::shared_ptr<Profiler> getInstance();
  void startProfiler();
  void stop();
  void traceStart();
  void traceEnd(jlong coroutine_id);
  TraceInfo &findOngoingTrace(const jlong &t_coroutineId);
  void removeOngoingTrace(const jlong &t_coroutineId);
  void coroutineCreated(jlong t_coroutineId);
  void coroutineSuspended(jlong coroutine_id);
  void coroutineResumed(jlong t_coroutineId);
  void coroutineCompleted(jlong coroutine_id);
 private:
  static std::shared_ptr<Profiler> instance;
  thread_local static jlong m_coroutineId;
  std::shared_ptr<JVM> m_jvm;
  std::unique_ptr<std::thread> m_profilerThread;
  AsyncGetCallTrace m_asyncTracePtr;
  TraceStorage m_storage;
  ConcurrentMap<jmethodID, std::shared_ptr<std::string>> m_methodInfoMap;
  std::atomic_bool m_active;
  std::chrono::nanoseconds m_threshold;

  Profiler(std::shared_ptr<JVM> t_jvm, std::chrono::nanoseconds t_threshold);
  void signal_action(int t_signo, siginfo_t *t_siginfo, void *t_ucontext);
  void processTraces();
  std::shared_ptr<std::string> processMethodInfo(jmethodID t_methodId,
                                                 jint t_lineno);
  static inline std::string tickToMessage(jint t_ticks);
  void printSuspensions(jlong t_coroutineId, std::stringstream &output);
};
}
#endif // KOTLIN_TRACER_PROFILER_H
