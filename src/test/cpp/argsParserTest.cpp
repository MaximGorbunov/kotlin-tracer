#include <gtest/gtest.h>
#include <filesystem>
#include "utils/argsParser.hpp"

using namespace std;
TEST(ArgsParserTest, TestMethodParsing) {
  auto args = "method=io/example/ClassName.methodName";
  const shared_ptr<kotlinTracer::ProfilerOptions> &options = kotlinTracer::ArgsParser::parseProfilerOptions(args);
  EXPECT_EQ(string("methodName"), *options->methodName);
  EXPECT_EQ(string("io/example/ClassName"), *options->className);
}

TEST(ArgsParserTest, TestPeriodParsing) {
  auto args = "period=1,method=io/example/ClassName.methodName";
  const shared_ptr<kotlinTracer::ProfilerOptions> &options = kotlinTracer::ArgsParser::parseProfilerOptions(args);
  EXPECT_EQ(string("methodName"), *options->methodName);
  EXPECT_EQ(string("io/example/ClassName"), *options->className);
  EXPECT_EQ(chrono::nanoseconds(1), options->profilingPeriod);
}

TEST(ArgsParserTest, TestJarParsing) {
  auto args = "period=1,method=io/example/ClassName.methodName,jarPath=somewhat";
  const shared_ptr<kotlinTracer::ProfilerOptions> &options = kotlinTracer::ArgsParser::parseProfilerOptions(args);
  EXPECT_EQ(string("methodName"), *options->methodName);
  EXPECT_EQ(string("io/example/ClassName"), *options->className);
  EXPECT_EQ(string("somewhat"), *options->jarPath);
  EXPECT_EQ(chrono::nanoseconds(1), options->profilingPeriod);
}

TEST(ArgsParserTest, TestThresholdParsing) {
  auto args = "period=1,method=io/example/ClassName.methodName,jarPath=somewhat,threshold=1000";
  const shared_ptr<kotlinTracer::ProfilerOptions> &options = kotlinTracer::ArgsParser::parseProfilerOptions(args);
  EXPECT_EQ(string("methodName"), *options->methodName);
  EXPECT_EQ(string("io/example/ClassName"), *options->className);
  EXPECT_EQ(string("somewhat"), *options->jarPath);
  EXPECT_EQ(chrono::nanoseconds(1), options->profilingPeriod);
  EXPECT_EQ(chrono::milliseconds(1000), options->threshold);
}

TEST(ArgsParserTest, TestOutputPathParsing) {
  auto args = "period=1,method=io/example/ClassName.methodName,jarPath=somewhat,threshold=1000,outputPath=/a/b/";
  const shared_ptr<kotlinTracer::ProfilerOptions> &options = kotlinTracer::ArgsParser::parseProfilerOptions(args);
  EXPECT_EQ(string("methodName"), *options->methodName);
  EXPECT_EQ(string("io/example/ClassName"), *options->className);
  EXPECT_EQ(string("somewhat"), *options->jarPath);
  EXPECT_EQ(chrono::nanoseconds(1), options->profilingPeriod);
  EXPECT_EQ(chrono::milliseconds(1000), options->threshold);
  EXPECT_EQ(string("/a/b/"), *options->outputPath);
}

TEST(ArgsParserTest, TestOutputPathDefault) {
  auto args = "period=1,method=io/example/ClassName.methodName,jarPath=somewhat,threshold=1000";
  const shared_ptr<kotlinTracer::ProfilerOptions> &options = kotlinTracer::ArgsParser::parseProfilerOptions(args);
  EXPECT_EQ(string("methodName"), *options->methodName);
  EXPECT_EQ(string("io/example/ClassName"), *options->className);
  EXPECT_EQ(string("somewhat"), *options->jarPath);
  EXPECT_EQ(chrono::nanoseconds(1), options->profilingPeriod);
  EXPECT_EQ(chrono::milliseconds(1000), options->threshold);
  EXPECT_EQ(std::filesystem::current_path().string(), *options->outputPath);
}