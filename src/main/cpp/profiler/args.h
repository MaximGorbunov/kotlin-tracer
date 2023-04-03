#ifndef KOTLIN_TRACER_ARGS_H
#define KOTLIN_TRACER_ARGS_H

#include <string>
#include <memory>
#include <chrono>

typedef struct profilerOptions {
  std::shared_ptr<std::string> className;
  std::shared_ptr<std::string> methodName;
  std::chrono::nanoseconds profilingPeriod;
} profilerOptions;

#endif //KOTLIN_TRACER_ARGS_H
