#include <gtest/gtest.h>
#include <filesystem>
#include "utils/argsParser.hpp"

using std::string, std::shared_ptr, std::chrono::nanoseconds, std::chrono::milliseconds;

namespace kotlin_tracer {
TEST(ArgsParserTest, TestMethodParsing) {
  auto args = "method=io/example/ClassName.methodName";
  const shared_ptr<kotlin_tracer::ProfilerOptions> &options = kotlin_tracer::ArgsParser::parseProfilerOptions(args);
  EXPECT_EQ(string("methodName"), *options->method_name);
  EXPECT_EQ(string("io/example/ClassName"), *options->class_name);
}

TEST(ArgsParserTest, TestPeriodParsing) {
  auto args = "period=1,method=io/example/ClassName.methodName";
  const shared_ptr<kotlin_tracer::ProfilerOptions> &options = kotlin_tracer::ArgsParser::parseProfilerOptions(args);
  EXPECT_EQ(string("methodName"), *options->method_name);
  EXPECT_EQ(string("io/example/ClassName"), *options->class_name);
  EXPECT_EQ(nanoseconds(1), options->profiling_period);
}

TEST(ArgsParserTest, TestJarParsing) {
  auto args = "period=1,method=io/example/ClassName.methodName,jarPath=somewhat";
  const shared_ptr<kotlin_tracer::ProfilerOptions> &options = kotlin_tracer::ArgsParser::parseProfilerOptions(args);
  EXPECT_EQ(string("methodName"), *options->method_name);
  EXPECT_EQ(string("io/example/ClassName"), *options->class_name);
  EXPECT_EQ(string("somewhat"), *options->jar_path);
  EXPECT_EQ(nanoseconds(1), options->profiling_period);
}

TEST(ArgsParserTest, TestThresholdParsing) {
  auto args = "period=1,method=io/example/ClassName.methodName,jarPath=somewhat,threshold=1000";
  const shared_ptr<kotlin_tracer::ProfilerOptions> &options = kotlin_tracer::ArgsParser::parseProfilerOptions(args);
  EXPECT_EQ(string("methodName"), *options->method_name);
  EXPECT_EQ(string("io/example/ClassName"), *options->class_name);
  EXPECT_EQ(string("somewhat"), *options->jar_path);
  EXPECT_EQ(nanoseconds(1), options->profiling_period);
  EXPECT_EQ(milliseconds(1000), options->threshold);
}

TEST(ArgsParserTest, TestOutputPathParsing) {
  auto args = "period=1,method=io/example/ClassName.methodName,jarPath=somewhat,threshold=1000,outputPath=/a/b/";
  const shared_ptr<kotlin_tracer::ProfilerOptions> &options = kotlin_tracer::ArgsParser::parseProfilerOptions(args);
  EXPECT_EQ(string("methodName"), *options->method_name);
  EXPECT_EQ(string("io/example/ClassName"), *options->class_name);
  EXPECT_EQ(string("somewhat"), *options->jar_path);
  EXPECT_EQ(nanoseconds(1), options->profiling_period);
  EXPECT_EQ(milliseconds(1000), options->threshold);
  EXPECT_EQ(string("/a/b/"), *options->output_path);
}

TEST(ArgsParserTest, TestOutputPathDefault) {
  auto args = "period=1,method=io/example/ClassName.methodName,jarPath=somewhat,threshold=1000";
  const shared_ptr<kotlin_tracer::ProfilerOptions> &options = kotlin_tracer::ArgsParser::parseProfilerOptions(args);
  EXPECT_EQ(string("methodName"), *options->method_name);
  EXPECT_EQ(string("io/example/ClassName"), *options->class_name);
  EXPECT_EQ(string("somewhat"), *options->jar_path);
  EXPECT_EQ(nanoseconds(1), options->profiling_period);
  EXPECT_EQ(milliseconds(1000), options->threshold);
  EXPECT_EQ(std::filesystem::current_path().string(), *options->output_path);
}
}  // namespace kotlin_tracer
