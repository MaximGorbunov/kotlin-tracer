#include <memory>
#include <stdexcept>
#include <filesystem>
#include <utility>

#include "argsParser.hpp"

using std::string, std::chrono::nanoseconds, std::chrono::milliseconds,
    std::unique_ptr, std::runtime_error;

namespace kotlin_tracer {
unique_ptr<ProfilerOptions> ArgsParser::parseProfilerOptions(
    const string &options
) {
  nanoseconds period(1000000);  // 1ms default
  nanoseconds threshold(0);
  unique_ptr<string> method;
  unique_ptr<string> klass;
  unique_ptr<string> jar_path;
  unique_ptr<string> output_path;
  size_t current_position = 0;
  size_t delimiter_position;
  do {
    delimiter_position = options.find(',', current_position);
    if (delimiter_position == string::npos) {
      delimiter_position = options.length();
    }
    auto keyValueDelimiterPosition = options.find('=', current_position);
    if (keyValueDelimiterPosition == string::npos) {
      throw runtime_error("Incorrect options provided: " + options +
          ". Use following syntax: method=x/y/z/Class.method,period=1000");
    }
    auto key = options.substr(current_position,
                              keyValueDelimiterPosition - current_position);
    string value = options.substr(keyValueDelimiterPosition + 1,
                                  delimiter_position - keyValueDelimiterPosition
                                      - 1);
    if (key == "method") {
      auto [className, methodName] = ArgsParser::parseMethod(value);
      method = std::move(methodName);
      klass = std::move(className);
    } else if (key == "period") {
      period = ArgsParser::parsePeriod(value);
    } else if (key == "jarPath") {
      jar_path = ArgsParser::parseJarPath(value);
    } else if (key == "outputPath") {
      output_path = ArgsParser::parseOutputPath(value);
    } else if (key == "threshold") {
      threshold = ArgsParser::parseThreshold(value);
    }
    current_position = delimiter_position + 1;
  } while (current_position < options.length());
  if (method == nullptr) {
    throw runtime_error(
        "Method name must be provided."
        "Use following syntax: method=x/y/z/Class.method,period=1000");
  }
  if (output_path == nullptr) {
    output_path = make_unique<string>(std::filesystem::current_path().string());
  }
  return make_unique<ProfilerOptions>(std::move(klass),
                                      std::move(method),
                                      period,
                                      threshold,
                                      std::move(jar_path),
                                      std::move(output_path));
}

ArgsParser::InstrumentationInfo ArgsParser::parseMethod(const string &option) {
  auto methodDelimiter = option.find('.');
  if (methodDelimiter == string::npos) {
    throw runtime_error(
        "Provided incorrect method: " + option
            + ". Use following syntax package/class.method");
  }
  return {make_unique<string>(option.substr(0, methodDelimiter)),
          make_unique<string>(option.substr(methodDelimiter + 1))};
}

nanoseconds ArgsParser::parsePeriod(const string &option) {
  return nanoseconds(stoi(option));
}

nanoseconds ArgsParser::parseThreshold(const string &option) {
  return duration_cast<nanoseconds>(milliseconds(stoi(option)));
}

unique_ptr<string> ArgsParser::parseJarPath(const string &option) {
  return make_unique<string>(option);
}

unique_ptr<string> ArgsParser::parseOutputPath(const std::string &option) {
  return make_unique<string>(option);
}
}  // namespace kotlin_tracer
