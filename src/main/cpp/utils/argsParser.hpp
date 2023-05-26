#ifndef KOTLIN_TRACER_ARGSPARSER_H
#define KOTLIN_TRACER_ARGSPARSER_H

#include <memory>
#include <string>

#include "../profiler/model/args.hpp"

namespace kotlinTracer {
class ArgsParser {
 public:
  static std::unique_ptr<ProfilerOptions> parseProfilerOptions(const std::string &t_options);
 private:
  static inline std::unique_ptr<ProfilerOptions> parseMethod(const std::string &t_option,
                                                             std::unique_ptr<ProfilerOptions> t_options);
  static inline std::unique_ptr<ProfilerOptions> parsePeriod(const std::string &option,
                                                             std::unique_ptr<ProfilerOptions> t_options);
  static inline std::unique_ptr<ProfilerOptions> parseJarPath(const std::string &option,
                                                              std::unique_ptr<ProfilerOptions> t_options);
};
}
#endif //KOTLIN_TRACER_ARGSPARSER_H
