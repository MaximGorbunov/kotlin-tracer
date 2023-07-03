#include <memory>
#include <stdexcept>
#include <filesystem>

#include "argsParser.hpp"

using std::string, std::chrono::nanoseconds, std::chrono::milliseconds, std::unique_ptr, std::runtime_error;

namespace kotlin_tracer {
unique_ptr<ProfilerOptions> ArgsParser::parseProfilerOptions(const string &options) {
  nanoseconds period(1000000);// 1ms default
  nanoseconds threshold(0);
  unique_ptr<string> method;
  unique_ptr<string> klass;
  unique_ptr<string> jarPath;
  unique_ptr<string> outputPath;
  size_t currentPosition = 0;
  size_t delimiterPosition;
  do {
    delimiterPosition = options.find(',', currentPosition);
    if (delimiterPosition == string::npos) {
      delimiterPosition = options.length();
    }
    auto keyValueDelimiterPosition = options.find('=', currentPosition);
    if (keyValueDelimiterPosition == string::npos) {
      throw runtime_error("Incorrect options provided: " + options +
          ". Use following syntax: method=x/y/z/Class.method,period=1000");
    }
    string key = options.substr(currentPosition, keyValueDelimiterPosition - currentPosition);
    string value = options.substr(keyValueDelimiterPosition + 1, delimiterPosition - keyValueDelimiterPosition - 1);
    if (key == "method") {
      auto [className, methodName] = ArgsParser::parseMethod(value);
      method = std::move(methodName);
      klass = std::move(className);
    } else if (key == "period") {
      period = ArgsParser::parsePeriod(value);
    } else if (key == "jarPath") {
      jarPath = ArgsParser::parseJarPath(value);
    } else if (key == "outputPath") {
      outputPath = ArgsParser::parseOutputPath(value);
    } else if (key == "threshold") {
      threshold = ArgsParser::parseThreshold(value);
    }
    currentPosition = delimiterPosition + 1;
  } while (currentPosition < options.length());
  if (method == nullptr) {
    throw runtime_error("Method name must be provided. Use following syntax: method=x/y/z/Class.method,period=1000");
  }
  if (outputPath == nullptr) {
    outputPath = make_unique<string>(std::filesystem::current_path().string());
  }
  return make_unique<ProfilerOptions>(std::move(klass), std::move(method), period, threshold,
                                      std::move(jarPath), std::move(outputPath));
}

ArgsParser::InstrumentationInfo ArgsParser::parseMethod(const string &option) {
  auto methodDelimiter = option.find('.');
  if (methodDelimiter == string::npos) {
    throw runtime_error(
        "Provided incorrect method: " + option + ". Use following syntax package/class.method");
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
}