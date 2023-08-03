#include <gtest/gtest.h>
#include "agent.hpp"
#include "jni/jni_instrumentator.h"
namespace kotlin_tracer {
TEST(JNIInstrumentatorTest, TestMethodParsing) {
  std::string methodInvoked;
  try {
    Java_io_inst_CoroutineInstrumentator_coroutineCreated(nullptr, nullptr, 1L);
  } catch (std::runtime_error &error) {
    methodInvoked = std::string(error.what());
  }
  EXPECT_EQ("coroutineCreated", methodInvoked);
}

TEST(JNIInstrumentatorTest, TestCoroutineResumed) {
  std::string methodInvoked;
  try {
    Java_io_inst_CoroutineInstrumentator_coroutineResumed(nullptr, nullptr, 1);
  } catch (std::runtime_error &error) {
    methodInvoked = std::string(error.what());
  }
  EXPECT_EQ("coroutineResumed", methodInvoked);
}

TEST(JNIInstrumentatorTest, TestcoroutineSuspended) {
  std::string methodInvoked;
  try {
    Java_io_inst_CoroutineInstrumentator_coroutineSuspend(nullptr, nullptr, 1);
  } catch (std::runtime_error &error) {
    methodInvoked = std::string(error.what());
  }
  EXPECT_EQ("coroutineSuspended", methodInvoked);
}

TEST(JNIInstrumentatorTest, TestcoroutineCompleted) {
  std::string methodInvoked;
  try {
    Java_io_inst_CoroutineInstrumentator_coroutineCompleted(nullptr, nullptr, 1);
  } catch (std::runtime_error &error) {
    methodInvoked = std::string(error.what());
  }
  EXPECT_EQ("coroutineCompleted", methodInvoked);
}

TEST(JNIInstrumentatorTest, TesttraceStart) {
  std::string methodInvoked;
  try {
    Java_io_inst_CoroutineInstrumentator_traceStart(nullptr, nullptr, false);
  } catch (std::runtime_error &error) {
    methodInvoked = std::string(error.what());
  }
  EXPECT_EQ("traceStart", methodInvoked);
}

TEST(JNIInstrumentatorTest, TesttraceEnd) {
  std::string methodInvoked;
  try {
    Java_io_inst_CoroutineInstrumentator_traceEnd(nullptr, nullptr, 1);
  } catch (std::runtime_error &error) {
    methodInvoked = std::string(error.what());
  }
  EXPECT_EQ("traceEnd", methodInvoked);
}
}  // namespace kotlin_tracer
