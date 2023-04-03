#include <gtest/gtest.h>
#include "argsParser.h"

using namespace std;
TEST(ArgsParserTest, TestMethodParsing) {
    ArgsParser parser;
    auto args = make_shared<string>("method=io/example/ClassName.methodName");
    const shared_ptr<profilerOptions> &options = parser.parseProfilerOptions(args);
    EXPECT_EQ(string("methodName"), *options->methodName);
    EXPECT_EQ(string("io/example/ClassName"), *options->className);
}

TEST(ArgsParserTest, TestPeriodParsing) {
    ArgsParser parser;
    auto args = make_shared<string>("period=1,method=io/example/ClassName.methodName");
    const shared_ptr<profilerOptions> &options = parser.parseProfilerOptions(args);
    EXPECT_EQ(string("methodName"), *options->methodName);
    EXPECT_EQ(string("io/example/ClassName"), *options->className);
    EXPECT_EQ(chrono::nanoseconds(1), options->profilingPeriod);
}