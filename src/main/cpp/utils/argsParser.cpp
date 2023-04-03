#include "argsParser.h"

#include <memory>

using namespace std;

shared_ptr<profilerOptions> ArgsParser::parseProfilerOptions(const shared_ptr<string> &options) {
  shared_ptr<profilerOptions> result = std::make_shared<profilerOptions>();
  result->profilingPeriod = chrono::nanoseconds(1000000);// 1ms default
  size_t currentPosition = 0;
  size_t delimiterPosition;
  do {
    delimiterPosition = options->find(',', currentPosition);
    if (delimiterPosition == string::npos) {
      delimiterPosition = options->length() - 1;
    }
    auto keyValueDelimiterPosition = options->find('=', currentPosition);
    if (keyValueDelimiterPosition == string::npos) {
      throw runtime_error("Incorrect options provided: " + *options +
          ". Use following syntax: method=x/y/z/Class.method,period=1000");
    }
    string key = options->substr(currentPosition, keyValueDelimiterPosition - currentPosition);
    string value = options->substr(keyValueDelimiterPosition + 1, delimiterPosition - keyValueDelimiterPosition + 1);
    if (key == "method") {
      ArgsParser::parseMethod(value, result);
    } else if (key == "period") {
      ArgsParser::parsePeriod(value, result);
    }
    currentPosition = delimiterPosition + 1;

  } while (currentPosition < options->length());
  if (result->methodName == nullptr) {
    throw runtime_error("Method name must be provided. Use following syntax: method=x/y/z/Class.method,period=1000");
  }
  return result;
}

void ArgsParser::parseMethod(const string &value, const shared_ptr<profilerOptions> &options) {
  auto methodDelimiter = value.find('.');
  if (methodDelimiter == string::npos) {
    throw runtime_error(
        "Provided incorrect method: " + value + ". Use following syntax package/class.method");
  }
  options->methodName = make_shared<string>(value.substr(methodDelimiter + 1));
  options->className = make_shared<string>(value.substr(0, methodDelimiter));
}

void ArgsParser::parsePeriod(const string &value, const shared_ptr<profilerOptions> &options) {
  auto intValue = stoi(value);
  options->profilingPeriod = chrono::nanoseconds(intValue);
}