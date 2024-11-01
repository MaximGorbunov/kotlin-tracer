#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_AGENT_HPP_
#define KOTLIN_TRACER_SRC_MAIN_CPP_AGENT_HPP_

#include <memory>
#include <thread>
#include <shared_mutex>

#include "vm/jvm.hpp"
#include "profiler/profiler.hpp"
#include "profiler/instrumentation.hpp"
#include "profiler/model/args.hpp"
#include "utils/log.h"

namespace kotlin_tracer {
class Agent {
 public:
  Agent(JavaVM* java_vm,
        std::unique_ptr<jvmtiEventCallbacks> callbacks,
        std::unique_ptr<ProfilerOptions> profiler_options)
      : jvm_(std::make_unique<JVM>(java_vm, std::move(callbacks))),
        profiler_(Profiler::create(jvm_.get(), profiler_options->threshold, *profiler_options->output_path, profiler_options->profiling_period)),
        instrumentation_(std::make_unique<Instrumentation>(std::move(profiler_options), jvm_.get())) {};

  ~Agent() {
    logDebug("[Kotlin-tracer] Unloading agent");
    profiler_->stop();
  }

  JVM* getJVM();
  Instrumentation* getInstrumentation();
  Profiler* getProfiler();

 private:
  std::unique_ptr<JVM> jvm_;
  std::unique_ptr<Profiler> profiler_;
  std::unique_ptr<Instrumentation> instrumentation_;
};

void coroutineCreated(jlong coroutine_id);
void coroutineResumed(jlong coroutine_id);
void coroutineSuspended(jlong coroutine_id);
void coroutineCompleted(jlong coroutine_id);
void traceStart(jlong coroutine_id);
void traceEnd(jlong coroutine_id);
}
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_AGENT_HPP_
