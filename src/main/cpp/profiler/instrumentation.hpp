#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_INSTRUMENTATION_HPP_
#define KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_INSTRUMENTATION_HPP_

#include <string>
#include <memory>
#include <utility>
#include <jni.h>

#include "../vm/jvm.hpp"
#include "model/args.hpp"

namespace kotlin_tracer {
class Instrumentation {
 public:
  explicit Instrumentation(std::unique_ptr<ProfilerOptions> t_profilerOptions, std::shared_ptr<JVM> t_jvm) :
      m_profilerOptions(std::move(t_profilerOptions)), m_jvm(std::move(t_jvm)) {};

  ~Instrumentation() = default;

  void instrument(JNIEnv *jniEnv, const char *t_name,
                  jint t_classDataLen,
                  const unsigned char *t_classData,
                  jint *t_newClassDataLen,
                  unsigned char **t_newClassData);
  void setInstrumentationMetadata(std::unique_ptr<InstrumentationMetadata> metadata);
  std::unique_ptr<std::string> getJarPath();
 private:
  std::unique_ptr<ProfilerOptions> m_profilerOptions;
  std::shared_ptr<JVM> m_jvm;
  static const std::string KOTLIN_COROUTINE_PROBES_NAME;
  std::unique_ptr<InstrumentationMetadata> m_instrumentMetadata;

  jbyteArray instrumentCoroutines(const unsigned char *byteCode, int size);
  jbyteArray instrumentMethod(const unsigned char *byteCode, int size, const std::string &methodName);
};
}
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_INSTRUMENTATION_HPP_
