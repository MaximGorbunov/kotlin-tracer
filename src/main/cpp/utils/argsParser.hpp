#ifndef KOTLIN_TRACER_ARGSPARSER_H
#define KOTLIN_TRACER_ARGSPARSER_H

#include <memory>
#include <string>

#include "../profiler/model/args.hpp"

namespace kotlin_tracer {

class ArgsParser {
 public:
  static std::unique_ptr<ProfilerOptions> parseProfilerOptions(const std::string &options);
 private:
  struct InstrumentationInfo {
    std::unique_ptr<std::string> class_name;
    std::unique_ptr<std::string> method_name;
  };
  static inline InstrumentationInfo parseMethod(const std::string &option);
  static inline std::chrono::nanoseconds parsePeriod(const std::string &option);
  static inline std::chrono::nanoseconds parseThreshold(const std::string &option);
  static inline std::unique_ptr<std::string> parseJarPath(const std::string &option);
  static inline std::unique_ptr<std::string> parseOutputPath(const std::string &option);
};
}
#endif //KOTLIN_TRACER_ARGSPARSER_H
