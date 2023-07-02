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

typedef void (*AsyncGetCallTrace)(ASGCTCallTrace *trace, jint depth, void *ucontext);
}
#endif