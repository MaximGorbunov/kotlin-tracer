#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_AGENT_HPP_
#define KOTLIN_TRACER_SRC_MAIN_CPP_AGENT_HPP_

#include <memory>
#include <thread>

#include "vm/jvm.hpp"
#include "profiler/profiler.hpp"
#include "profiler/instrumentation.hpp"
#include "profiler/model/args.hpp"

// Required to be enabled for AsyncTrace usage
namespace kotlinTracer {
class Agent {
 public:
  Agent(JavaVM *t_vm,
        std::unique_ptr<jvmtiEventCallbacks> t_callbacks,
        std::unique_ptr<ProfilerOptions> t_profilerOptions)
      : m_jvm(std::make_shared<JVM>(t_vm, t_callbacks.get())),
        m_profiler(Profiler::create(m_jvm)),
        m_instrumentation(std::make_unique<Instrumentation>(std::move(t_profilerOptions), m_jvm)) {};

  ~Agent() {
    logDebug("[Kotlin-tracer] Unloading agent");
    m_profiler->stop();
  }

  std::shared_ptr<JVM> getJVM();
  std::shared_ptr<Instrumentation> getInstrumentation();
  std::shared_ptr<Profiler> getProfiler();

 private:
  std::shared_ptr<JVM> m_jvm;
  std::shared_ptr<Profiler> m_profiler;
  std::shared_ptr<Instrumentation> m_instrumentation;
};

extern std::unique_ptr<Agent> agent;
}
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_AGENT_HPP_
