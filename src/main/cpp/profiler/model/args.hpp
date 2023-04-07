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
} ProfilerOptions;
}
#endif //KOTLIN_TRACER_ARGS_H
