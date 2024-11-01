#ifndef TRACE_TIME_H
#define TRACE_TIME_H

#include <sys/time.h>
#include <chrono>
#include <atomic>

namespace kotlin_tracer {
struct TraceTime {
  std::atomic_llong value;
  explicit TraceTime(long long init_value) : value(init_value) {}
  TraceTime(const TraceTime &trace_time) {
    value.store(trace_time.value, std::memory_order_relaxed);
  }
  TraceTime operator-(const TraceTime &other) {
    return TraceTime{value - other.value};
  }
  bool operator>=(const TraceTime &other) {
    return value >= other.value;
  }
  bool operator>(const TraceTime &other) {
    return value > other.value;
  }
  bool operator<=(const TraceTime &other) {
    return value <= other.value;
  }
  bool operator<(const TraceTime &other) {
    return value < other.value;
  }
  bool operator==(const TraceTime &other) {
    return value == other.value;
  }
  bool operator==(long long other) {
    return value == other;
  }
  TraceTime &operator=(long long new_value) {
    value.store(new_value, std::memory_order_relaxed);
    return *this;
  }
  TraceTime &operator=(const TraceTime& new_value) {
    value.store(new_value.value, std::memory_order_relaxed);
    return *this;
  }
  TraceTime &operator+=(const TraceTime &new_value) {
    value.fetch_add(new_value.value, std::memory_order_relaxed);
    return *this;
  }
  long long get() {
    return value.load(std::memory_order_relaxed);
  }
  void set(long long new_value) {
    value.store(new_value, std::memory_order_relaxed);
  }
};

TraceTime currentTimeMs();
TraceTime currentTimeNs();
}
#endif
