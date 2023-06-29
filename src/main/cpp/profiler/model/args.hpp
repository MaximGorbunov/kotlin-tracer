#ifndef KOTLIN_TRACER_ARGS_H
#define KOTLIN_TRACER_ARGS_H

#include <string>
#include <memory>
#include <chrono>

namespace kotlinTracer {
typedef struct ProfilerOptions {
  ProfilerOptions(
      std::unique_ptr<std::string> t_className,
      std::unique_ptr<std::string> t_methodName,
      std::chrono::nanoseconds t_profilingPeriod,
      std::chrono::nanoseconds t_threshold,
      std::unique_ptr<std::string> t_jarPath,
      std::unique_ptr<std::string> t_outputPath
  ) : className(std::move(t_className)),
      methodName(std::move(t_methodName)),
      profilingPeriod(t_profilingPeriod),
      threshold(t_threshold),
      jarPath(std::move(t_jarPath)),
      outputPath(std::move(t_outputPath)) {}

  const std::unique_ptr<std::string> className;
  const std::unique_ptr<std::string> methodName;
  const std::chrono::nanoseconds profilingPeriod;
  const std::chrono::nanoseconds threshold;
  const std::unique_ptr<std::string> jarPath;
  const std::unique_ptr<std::string> outputPath;
} ProfilerOptions;
}
#endif //KOTLIN_TRACER_ARGS_H
