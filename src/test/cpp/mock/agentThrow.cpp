#include "agent.hpp"

namespace kotlin_tracer {
void coroutineCreated(jlong coroutine_id) {
  throw std::runtime_error("coroutineCreated");
}
void coroutineResumed(jlong coroutine_id) {
  throw std::runtime_error("coroutineResumed");
}
void coroutineSuspended(jlong coroutine_id) {
  throw std::runtime_error("coroutineSuspended");
}
void coroutineCompleted(jlong coroutine_id) {
  throw std::runtime_error("coroutineCompleted");
}
void traceStart(jlong coroutine_id) {
  throw std::runtime_error("traceStart");
}
void traceEnd(jlong coroutine_id) {
  throw std::runtime_error("traceEnd");
}
}  // namespace kotlin_tracer
