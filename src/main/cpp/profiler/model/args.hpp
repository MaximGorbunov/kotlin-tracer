#ifndef KOTLIN_TRACER_ARGS_H
#define KOTLIN_TRACER_ARGS_H

#include <string>
#include <memory>
#include <chrono>

namespace kotlin_tracer {
typedef struct ProfilerOptions {
  ProfilerOptions(
      std::unique_ptr<std::string> t_class_name,
      std::unique_ptr<std::string> t_method_name,
      std::chrono::nanoseconds t_profiling_period,
      std::chrono::nanoseconds t_threshold,
      std::unique_ptr<std::string> t_jar_path,
      std::unique_ptr<std::string> t_output_path
  ) : class_name(std::move(t_class_name)),
      method_name(std::move(t_method_name)),
      profiling_period(t_profiling_period),
      threshold(t_threshold),
      jar_path(std::move(t_jar_path)),
      output_path(std::move(t_output_path)) {}

  const std::unique_ptr<std::string> class_name;
  const std::unique_ptr<std::string> method_name;
  const std::chrono::nanoseconds profiling_period;
  const std::chrono::nanoseconds threshold;
  const std::unique_ptr<std::string> jar_path;
  const std::unique_ptr<std::string> output_path;
} ProfilerOptions;
}
#endif //KOTLIN_TRACER_ARGS_H
