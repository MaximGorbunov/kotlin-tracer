#include <memory>
#include <stdexcept>
#include <filesystem>

#include "argsParser.hpp"

using namespace std;

unique_ptr<kotlinTracer::ProfilerOptions> kotlinTracer::ArgsParser::parseProfilerOptions(const string &t_options) {
  chrono::nanoseconds period(1000000);// 1ms default
  chrono::nanoseconds threshold(0);
  unique_ptr<string> method;
  unique_ptr<string> klass;
  unique_ptr<string> jarPath;
  unique_ptr<string> outputPath;
  size_t currentPosition = 0;
  size_t delimiterPosition;
  do {
    delimiterPosition = t_options.find(',', currentPosition);
    if (delimiterPosition == string::npos) {
      delimiterPosition = t_options.length();
    }
    auto keyValueDelimiterPosition = t_options.find('=', currentPosition);
    if (keyValueDelimiterPosition == string::npos) {
      throw runtime_error("Incorrect options provided: " + t_options +
          ". Use following syntax: method=x/y/z/Class.method,period=1000");
    }
    string key = t_options.substr(currentPosition, keyValueDelimiterPosition - currentPosition);
    string value = t_options.substr(keyValueDelimiterPosition + 1, delimiterPosition - keyValueDelimiterPosition - 1);
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
  } while (currentPosition < t_options.length());
  if (method == nullptr) {
    throw runtime_error("Method name must be provided. Use following syntax: method=x/y/z/Class.method,period=1000");
  }
  if (outputPath == nullptr) {
    outputPath = make_unique<string>(std::filesystem::current_path().string());
  }
  return make_unique<ProfilerOptions>(std::move(klass), std::move(method), period, threshold,
                                                         std::move(jarPath), std::move(outputPath));
}

std::tuple<std::unique_ptr<string>,
           std::unique_ptr<string>> kotlinTracer::ArgsParser::parseMethod(const string &t_option) {
  auto methodDelimiter = t_option.find('.');
  if (methodDelimiter == string::npos) {
    throw runtime_error(
        "Provided incorrect method: " + t_option + ". Use following syntax package/class.method");
  }
  return {make_unique<string>(t_option.substr(0, methodDelimiter)),
          make_unique<string>(t_option.substr(methodDelimiter + 1))};
}

chrono::nanoseconds kotlinTracer::ArgsParser::parsePeriod(const string &value) {
  return chrono::nanoseconds(stoi(value));
}

chrono::nanoseconds kotlinTracer::ArgsParser::parseThreshold(const string &value) {
  return duration_cast<chrono::nanoseconds>(chrono::milliseconds(stoi(value)));
}

unique_ptr<string> kotlinTracer::ArgsParser::parseJarPath(const string &option) {
  return make_unique<string>(option);
}

unique_ptr<string> kotlinTracer::ArgsParser::parseOutputPath(const std::string &option) {
  return make_unique<string>(option);
}
