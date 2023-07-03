#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_INSTRUMENTATION_HPP_
#define KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_INSTRUMENTATION_HPP_

#include <string>
#include <memory>
#include <utility>
#include <jni.h>

#include "../vm/jvm.hpp"
#include "model/args.hpp"

#define KOTLIN_COROUTINE_PROBES_NAME "kotlinx/coroutines/debug/internal/DebugProbesImpl"

namespace kotlin_tracer {
class Instrumentation {
 public:
  explicit Instrumentation(std::unique_ptr<ProfilerOptions> profiler_options, std::shared_ptr<JVM> jvm) :
      profiler_options_(std::move(profiler_options)), jvm_(std::move(jvm)) {};

  ~Instrumentation() = default;

  void instrument(JNIEnv *jni_env, const char *name,
                  jint class_data_len,
                  const unsigned char *class_data,
                  jint *new_class_data_len,
                  unsigned char **new_class_data);
  void setInstrumentationMetadata(std::unique_ptr<InstrumentationMetadata> metadata);
  std::unique_ptr<std::string> getJarPath();
 private:
  std::unique_ptr<ProfilerOptions> profiler_options_;
  std::shared_ptr<JVM> jvm_;
  std::unique_ptr<InstrumentationMetadata> instrumentation_metadata_;

  jbyteArray instrumentCoroutines(const unsigned char *byte_code, int size);
  jbyteArray instrumentMethod(const unsigned char *byte_code, int size, const std::string &method_name);
};
}
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_INSTRUMENTATION_HPP_
