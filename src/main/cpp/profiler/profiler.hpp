#ifndef KOTLIN_TRACER_PROFILER_H
#define KOTLIN_TRACER_PROFILER_H

#include <csignal>
#include <unordered_map>

#include "model/asyncTrace.hpp"
#include "trace/traceStorage.hpp"
#include "../vm/jvm.hpp"

namespace kotlinTracer {
class Profiler {
 public:
  static std::shared_ptr<Profiler> create(std::shared_ptr<JVM> t_jvm);
  static std::shared_ptr<Profiler> getInstance();
  void startProfiler();
  void stop();
  void traceStart(jlong t_coroutineId);
  void traceEnd(jlong t_coroutineId);
  TraceInfo &findOngoingTrace(const jlong &t_coroutineId);
  TraceInfo &findCompletedTrace(const jlong &t_coroutineId);
  void removeOngoingTrace(const jlong &t_coroutineId);
  void coroutineCreated(jlong t_coroutineId);
  void coroutineSuspended(jlong t_coroutineId);
  void coroutineResumed(jlong t_coroutineId);
  void coroutineCompleted(jlong t_coroutineId);
 private:
  static std::shared_ptr<Profiler> instance;
  thread_local static jlong m_coroutineId;
  std::shared_ptr<JVM> m_jvm;
  std::unique_ptr<std::thread> m_profilerThread;
  AsyncGetCallTrace m_asyncTracePtr;
  TraceStorage m_storage;
  std::unordered_map<jmethodID, std::shared_ptr<std::string>> m_methodInfoMap;
  bool m_active;

  explicit Profiler(std::shared_ptr<JVM> t_jvm);
  void signal_action(int t_signo, siginfo_t *t_siginfo, void *t_ucontext);
  void processTraces();
  std::shared_ptr<std::string> processMethodInfo(jmethodID t_methodId,
                                                 jint t_lineno);
  static inline std::string tickToMessage(jint t_ticks);
};
}
#endif // KOTLIN_TRACER_PROFILER_H
