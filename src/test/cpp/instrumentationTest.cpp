#include <gtest/gtest.h>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "config.h"

using namespace std;

TEST(InstrumentationTest, CheckCoroutinesDebugProbesInstrumentation) {
  auto tempFilePath = std::filesystem::temp_directory_path() / "test.txt";
  auto agentPath = PROJECT_SOURCE_DIR + "/cmake-build-release/agent/libagent.dylib";
  system((GRADLEW_PATH
      + " -p " + PROJECT_SOURCE_DIR
      + " -Pmethod=io/github/maximgorbunov/InstrumentationTest.switchTableSuspend test "
      + "--tests io.github.maximgorbunov.InstrumentationTest.simpleFunctionSwitchTable "
      +  " > "
      + tempFilePath.string()).c_str());
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
    EXPECT_GT(elapsedTime, 100000000);
    EXPECT_LT(elapsedTime, 200000000);

  } else {
    throw runtime_error("Can't open temp file");
  }

}