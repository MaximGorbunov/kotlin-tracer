#include <gtest/gtest.h>
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