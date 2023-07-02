#include "traceTime.hpp"
#include <chrono>

using namespace std;

namespace kotlin_tracer {
kotlin_tracer::TraceTime currentTimeMs() {
  auto time = chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
}

kotlin_tracer::TraceTime currentTimeNs() {
  auto time = chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();
}
}