#include "traceTime.hpp"
#include <chrono>

using namespace std;

namespace kotlinTracer {
kotlinTracer::TraceTime currentTimeMs() {
  auto time = chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
}

kotlinTracer::TraceTime currentTimeNs() {
  auto time = chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();
}
}