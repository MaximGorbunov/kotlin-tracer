#ifndef KOTLIN_TRACER_SRC_TEST_CPP_INSTRUMENTATION_TEST_HPP_
#define KOTLIN_TRACER_SRC_TEST_CPP_INSTRUMENTATION_TEST_HPP_

#include <gtest/gtest.h>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "config.h"

namespace kotlin_tracer {
inline void checkExecutionTime(std::shared_ptr<std::filesystem::path> &t_temp_file_path, int t_more_than, int t_less_than) {
  std::ifstream log(*t_temp_file_path);
  std::string content;
  auto elapsedTime = -1L;
  if (log.is_open()) {
    while (log) {
      getline(log, content);
      std::cout << content << std::endl;
      if (content.find("trace end:") != std::string::npos) {
        auto timeStr = std::string("time: ");
        auto timePosition = content.find(timeStr);
        if (timePosition == std::string::npos) throw std::runtime_error("can't find time in log");
        elapsedTime = stol(content.substr(timePosition + timeStr.length()));
        std::cout << elapsedTime << std::endl;
      }
    }
    log.close();
    EXPECT_GT(elapsedTime, t_more_than);
    EXPECT_LT(elapsedTime, t_less_than);
  } else {
    throw std::runtime_error("Can't open temp file");
  }
}

inline std::shared_ptr<std::filesystem::path> runJVMTest(const std::string &t_intercept_method,
                                                         const std::string &t_test_name) {
  auto temp_file_path = std::make_shared<std::filesystem::path>(std::filesystem::temp_directory_path() / "test.txt");
  system((GRADLEW_PATH
      + " -p " + PROJECT_SOURCE_DIR
      + " -Pagent=" + AGENT_PATH
      + " -Pmethod=" + t_intercept_method + " test "
      + " -PoutputPath=" + std::filesystem::temp_directory_path().string() + " "
      + "--tests " + t_test_name
      + " > " + temp_file_path->string()).c_str());
  return temp_file_path;
}
}
#endif //KOTLIN_TRACER_SRC_TEST_CPP_INSTRUMENTATION_TEST_HPP_
