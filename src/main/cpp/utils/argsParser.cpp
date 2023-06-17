#include <memory>
#include <stdexcept>

#include "argsParser.hpp"

using namespace std;

unique_ptr<kotlinTracer::ProfilerOptions> kotlinTracer::ArgsParser::parseProfilerOptions(const string &t_options) {
  auto result = make_unique<ProfilerOptions>();
  result->profilingPeriod = chrono::nanoseconds(1000000);// 1ms default
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
      result = ArgsParser::parseMethod(value, std::move(result));
    } else if (key == "period") {
      result = ArgsParser::parsePeriod(value, std::move(result));
    } else if (key == "jarPath") {
      result = ArgsParser::parseJarPath(value, std::move(result));
    }
    currentPosition = delimiterPosition + 1;

  } while (currentPosition < t_options.length());
  if (result->methodName == nullptr) {
    throw runtime_error("Method name must be provided. Use following syntax: method=x/y/z/Class.method,period=1000");
  }
  return result;
}

unique_ptr<kotlinTracer::ProfilerOptions> kotlinTracer::ArgsParser::parseMethod(const string &t_option,
                                                                                unique_ptr<ProfilerOptions> t_options) {
  auto methodDelimiter = t_option.find('.');
  if (methodDelimiter == string::npos) {
    throw runtime_error(
        "Provided incorrect method: " + t_option + ". Use following syntax package/class.method");
  }
  t_options->methodName = make_unique<string>(t_option.substr(methodDelimiter + 1));
  t_options->className = make_unique<string>(t_option.substr(0, methodDelimiter));
  return t_options;
}

unique_ptr<kotlinTracer::ProfilerOptions> kotlinTracer::ArgsParser::parsePeriod(const string &value,
                                                                                unique_ptr<ProfilerOptions> t_options) {
  auto intValue = stoi(value);
  t_options->profilingPeriod = chrono::nanoseconds(intValue);
  return t_options;
}

unique_ptr<kotlinTracer::ProfilerOptions> kotlinTracer::ArgsParser::parseJarPath(const string &option,
                                                                                 unique_ptr<ProfilerOptions> t_options) {
  t_options->jarPath = make_unique<string>(option);
  return t_options;
}
