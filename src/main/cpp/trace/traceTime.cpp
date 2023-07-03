#include "traceTime.hpp"
#include <chrono>

namespace kotlin_tracer {
kotlin_tracer::TraceTime currentTimeMs() {
  auto time = std::chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
}

kotlin_tracer::TraceTime currentTimeNs() {
  auto time = std::chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();
}
}