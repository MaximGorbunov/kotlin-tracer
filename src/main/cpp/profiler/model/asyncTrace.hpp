#include <jvmti.h>

#ifndef TRACE_H
#define TRACE_H

namespace kotlin_tracer {
typedef struct {
  jint line_number;         // line number in the source file
  jmethodID method_id; // method executed in this frame
} ASGCTCallFrame;

typedef struct {
  JNIEnv *env_id;          // Env where trace was recorded
  jint num_frames;         // number of frames in this trace
  ASGCTCallFrame *frames; // frames
} ASGCTCallTrace;

typedef struct InstructionInfo {
  intptr_t instruction;
  uint64_t frame;
  bool java_frame;
} InstructionInfo;

typedef struct AsyncTrace {
  std::unique_ptr<InstructionInfo[]> instructions{};
  int64_t size{};
  pthread_t thread_id;
  std::atomic_flag ready{false};
  long coroutine_id = -1;
  AsyncTrace(std::unique_ptr<InstructionInfo[]> _instructions, uint64_t _size, pthread_t thread) : instructions(std::move(_instructions)), size(_size), thread_id(thread) {};
} AsyncTrace;

typedef void (*AsyncGetCallTrace)(ASGCTCallTrace *trace, jint depth, void *ucontext);
}
#endif