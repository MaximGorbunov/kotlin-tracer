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

// Required to be enabled for AsyncTrace usage
namespace kotlin_tracer {
class Agent {
 public:
  Agent(const std::shared_ptr<JavaVM>& java_vm,
        std::unique_ptr<jvmtiEventCallbacks> callbacks,
        std::unique_ptr<ProfilerOptions> profiler_options)
      : jvm_(std::make_shared<JVM>(java_vm, callbacks.get())),
        profiler_(Profiler::create(jvm_, profiler_options->threshold)),
        instrumentation_(std::make_unique<Instrumentation>(std::move(profiler_options), jvm_)) {};

  ~Agent() {
    logDebug("[Kotlin-tracer] Unloading agent");
    profiler_->stop();
  }

  std::shared_ptr<JVM> getJVM();
  std::shared_ptr<Instrumentation> getInstrumentation();
  std::shared_ptr<Profiler> getProfiler();

 private:
  std::shared_ptr<JVM> jvm_;
  std::shared_ptr<Profiler> profiler_;
  std::shared_ptr<Instrumentation> instrumentation_;
};

void coroutineCreated(jlong coroutine_id);
void coroutineResumed(jlong coroutine_id);
void coroutineSuspended(jlong coroutine_id);
void coroutineCompleted(jlong coroutine_id);
void traceStart();
void traceEnd(jlong coroutine_id);
}
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_AGENT_HPP_
