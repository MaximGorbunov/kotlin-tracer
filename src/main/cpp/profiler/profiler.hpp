#ifndef KOTLIN_TRACER_PROFILER_H
#define KOTLIN_TRACER_PROFILER_H

#include <csignal>
#include <unordered_map>

#include "model/asyncTrace.hpp"
#include "trace/traceStorage.hpp"
#include "../vm/jvm.hpp"
#include "concurrentCollections/concurrentVector.h"

#define NOT_FOUND (-1)

namespace kotlin_tracer {

class Profiler {
 public:
  static std::shared_ptr<Profiler> create(
      std::shared_ptr<JVM> jvm,
      std::chrono::nanoseconds threshold,
      const std::string &output_path,
      std::chrono::nanoseconds interval);
  static std::shared_ptr<Profiler> getInstance();
  void startProfiler();
  void stop();
  void traceStart(jlong coroutine_id);
  void traceEnd(jlong coroutine_id);
  TraceInfo &findOngoingTrace(const jlong &coroutine_id);
  void removeOngoingTrace(const jlong &coroutine_id);
  void coroutineCreated(jlong coroutine_id);
  void coroutineSuspended(jlong coroutine_id);
  void coroutineResumed(jlong coroutine_id);
  void coroutineCompleted(jlong coroutine_id);
  void gcStart();
  void gcFinish();
 private:
  std::unique_ptr<std::thread> profiler_thread_;
  static std::shared_ptr<Profiler> instance_;
  std::shared_ptr<JVM> jvm_;
  AsyncGetCallTrace async_trace_ptr_;
  TraceStorage storage_;
  ConcurrentCleanableMap<jmethodID, std::shared_ptr<std::string>> method_info_map_;
  ConcurrentCleanableMap<jmethodID , std::shared_ptr<std::string>> class_info_map_;
  std::atomic_flag active_;
  std::chrono::nanoseconds threshold_;
  std::chrono::nanoseconds interval_;
  std::string output_path_;
  std::unique_ptr<ConcurrentVector<std::shared_ptr<AsyncTrace>>> traces_;
  std::atomic_int trace_counter{0};
  std::atomic_int output_counter{0};

  Profiler(std::shared_ptr<JVM> jvm,
           std::chrono::nanoseconds threshold,
           std::string output_path,
           std::chrono::nanoseconds interval);
  void signal_action(int signo, siginfo_t *siginfo, void *ucontext);
  void processTraces();
  std::unique_ptr<StackFrame> processMethodInfo(jmethodID methodId);
  std::unique_ptr<StackFrame> processProfilerMethodInfo(const InstructionInfo &instruction_info);
  std::string unknown_native = std::string("unknown_native");
  std::string unknown_java = std::string("unknown_java");
  std::string empty = std::string("");
};
}
#endif // KOTLIN_TRACER_PROFILER_H
