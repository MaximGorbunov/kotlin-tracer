#include "instrumentation_test.hpp"

using std::string;

namespace kotlin_tracer {
TEST(InstrumentationTest, CheckCoroutinesDebugProbesInstrumentationForSwitchTable) {
  auto temp_file_path = runJVMTest("io/github/maximgorbunov/InstrumentationTest.switchTableSuspend",
                                   "io.github.maximgorbunov.InstrumentationTest.simpleFunctionSwitchTableTest");
  checkExecutionTime(temp_file_path, 100000000, 200000000);
  std::filesystem::remove(temp_file_path->string());
}

TEST(InstrumentationTest, CheckCoroutinesDebugProbesInstrumentationForComplexSwitchTable) {
  auto temp_file_path = runJVMTest("io/github/maximgorbunov/InstrumentationTest.switchTableComplexSuspend",
                                   "io.github.maximgorbunov.InstrumentationTest.complexFunctionSwitchTableTest");
  checkExecutionTime(temp_file_path, 200000000, 300000000);
  std::filesystem::remove(temp_file_path->string());
}

TEST(InstrumentationTest, CheckCoroutinesDebugProbesInstrumentationWithoutSwitchTable) {
  auto temp_file_path = runJVMTest("io/github/maximgorbunov/InstrumentationTest.suspendWithoutTable",
                                   "io.github.maximgorbunov.InstrumentationTest.simpleFunctionWithoutSwitchTableTest");
  checkExecutionTime(temp_file_path, 100000000, 200000000);
  std::filesystem::remove(temp_file_path->string());
}

TEST(InstrumentationTest, CheckPlainFunctionInstrumentation) {
  auto temp_file_path = runJVMTest("io/github/maximgorbunov/InstrumentationTest.plainFunction",
                                   "io.github.maximgorbunov.InstrumentationTest.plainFunctionTest");
  checkExecutionTime(temp_file_path, 100000000, 200000000);
  std::filesystem::remove(temp_file_path->string());
}

}  // namespace kotlin_tracer
