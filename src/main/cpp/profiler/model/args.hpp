#ifndef KOTLIN_TRACER_ARGS_H
#define KOTLIN_TRACER_ARGS_H

#include <string>
#include <memory>
#include <chrono>

namespace kotlinTracer {
typedef struct {
  std::unique_ptr<std::string> className;
  std::unique_ptr<std::string> methodName;
  std::chrono::nanoseconds profilingPeriod;
  std::chrono::nanoseconds threshold;
  std::unique_ptr<std::string> jarPath;
  std::unique_ptr<std::string> outputPath;
} ProfilerOptions;
}
#endif //KOTLIN_TRACER_ARGS_H
