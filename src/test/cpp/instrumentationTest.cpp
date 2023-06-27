#include <gtest/gtest.h>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "config.h"

using namespace std;

inline void checkExecutionTime(const std::filesystem::path &tempFilePath, int moreThan, int lessThan);
inline void runJVMTest(const std::string& interceptMethod, const std::string& testName, std::filesystem::path &tempFilePath);

TEST(InstrumentationTest, CheckCoroutinesDebugProbesInstrumentationForSwitchTable) {
  auto tempFilePath = std::filesystem::temp_directory_path() / "test.txt";
  runJVMTest("io/github/maximgorbunov/InstrumentationTest.switchTableSuspend",
             "io.github.maximgorbunov.InstrumentationTest.simpleFunctionSwitchTableTest",
             tempFilePath);
  checkExecutionTime(tempFilePath, 100000000, 200000000);
  std::filesystem::remove(tempFilePath.string());
}

TEST(InstrumentationTest, CheckCoroutinesDebugProbesInstrumentationForComplexSwitchTable) {
  auto tempFilePath = std::filesystem::temp_directory_path() / "test.txt";
  runJVMTest("io/github/maximgorbunov/InstrumentationTest.switchTableComplexSuspend",
             "io.github.maximgorbunov.InstrumentationTest.complexFunctionSwitchTableTest",
             tempFilePath);
  checkExecutionTime(tempFilePath, 200000000, 300000000);
  std::filesystem::remove(tempFilePath.string());
}

TEST(InstrumentationTest, CheckCoroutinesDebugProbesInstrumentationWithoutSwitchTable) {
  auto tempFilePath = std::filesystem::temp_directory_path() / "test.txt";
  runJVMTest("io/github/maximgorbunov/InstrumentationTest.suspendWithoutTable",
             "io.github.maximgorbunov.InstrumentationTest.simpleFunctionWithoutSwitchTableTest",
             tempFilePath);
  checkExecutionTime(tempFilePath, 100000000, 200000000);
  std::filesystem::remove(tempFilePath.string());
}

TEST(InstrumentationTest, CheckPlainFunctionInstrumentation) {
  auto tempFilePath = std::filesystem::temp_directory_path() / "test.txt";
  runJVMTest("io/github/maximgorbunov/InstrumentationTest.plainFunction",
             "io.github.maximgorbunov.InstrumentationTest.plainFunctionTest",
             tempFilePath);
  checkExecutionTime(tempFilePath, 100000000, 200000000);
  std::filesystem::remove(tempFilePath.string());
}

inline void checkExecutionTime(const std::filesystem::path &tempFilePath, int moreThan, int lessThan) {
  ifstream log(tempFilePath);
  string content;
  auto elapsedTime = -1;
  if (log.is_open()) {
    while (log) {
      getline(log, content);
      cout << content << endl;
      if (content.find("trace end:") != string::npos) {
        auto timeStr = string("time: ");
        auto timePosition = content.find(timeStr);
        if (timePosition == string::npos) throw runtime_error("can't find time in log");
        elapsedTime = stoi(content.substr(timePosition + timeStr.length()));
        cout << elapsedTime << endl;
      }
    }
    log.close();
    EXPECT_GT(elapsedTime, moreThan);
    EXPECT_LT(elapsedTime, lessThan);
  } else {
    throw runtime_error("Can't open temp file");
  }
}

inline void runJVMTest(const std::string& interceptMethod, const std::string& testName, std::filesystem::path &tempFilePath) {
  system((GRADLEW_PATH
      + " -p " + PROJECT_SOURCE_DIR
      + " -Pagent=" + AGENT_PATH
      + " -Pmethod=" + interceptMethod + " test "
      + ""
      + "--tests " + testName
      + " > " + tempFilePath.string()).c_str());
}

