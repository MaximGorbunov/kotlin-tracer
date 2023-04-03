#include "traceTime.h"
#include <chrono>

using namespace std;

trace_time current_time_ms() {
  auto time = chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
}

trace_time current_time_ns() {
  auto time = chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();
}
