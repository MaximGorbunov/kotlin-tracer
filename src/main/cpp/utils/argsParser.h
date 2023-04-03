#ifndef KOTLIN_TRACER_ARGSPARSER_H
#define KOTLIN_TRACER_ARGSPARSER_H

#include "args.h"
#include <memory>
#include <string>

class ArgsParser {
 public:
  static std::shared_ptr<profilerOptions> parseProfilerOptions(const std::shared_ptr<std::string> &options);
 private:
  static void parseMethod(const std::string &option, const std::shared_ptr<profilerOptions> &options);
  static void parsePeriod(const std::string &option, const std::shared_ptr<profilerOptions> &options);
};

#endif //KOTLIN_TRACER_ARGSPARSER_H
