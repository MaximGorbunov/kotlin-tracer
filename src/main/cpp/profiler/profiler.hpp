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
  static std::shared_ptr<Profiler> create(std::shared_ptr<JVM> jvm, std::chrono::nanoseconds threshold, const std::string &output_path);
  static std::shared_ptr<Profiler> getInstance();
  void startProfiler();
  void stop();
  void traceStart();
  void traceEnd(jlong coroutine_id);
  TraceInfo &findOngoingTrace(const jlong &coroutine_id);
  void removeOngoingTrace(const jlong &coroutine_id);
  void coroutineCreated(jlong coroutine_id);
  void coroutineSuspended(jlong coroutine_id);
  void coroutineResumed(jlong coroutine_id);
  void coroutineCompleted(jlong coroutine_id);
 private:
  static std::shared_ptr<Profiler> instance_;
  thread_local static jlong coroutine_id_;
  std::shared_ptr<JVM> jvm_;
  std::unique_ptr<std::thread> profiler_thread_;
  AsyncGetCallTrace async_trace_ptr_;
  TraceStorage storage_;
  ConcurrentMap<jmethodID, std::shared_ptr<std::string>> method_info_map_;
  std::atomic_bool active_;
  std::chrono::nanoseconds threshold_;
  std::chrono::milliseconds interval_;
  std::string output_path_;

  Profiler(std::shared_ptr<JVM> jvm, std::chrono::nanoseconds threshold, std::string output_path);
  void signal_action(int signo, siginfo_t *siginfo, void *ucontext);
  void processTraces();
  std::shared_ptr<std::string> processMethodInfo(jmethodID methodId,
                                                 jint lineno);
  static inline std::string tickToMessage(jint ticks);
};
}
#endif // KOTLIN_TRACER_PROFILER_H
