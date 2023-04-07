#include <jvmti.h>

#ifndef TRACE_H
#define TRACE_H

namespace kotlinTracer {
typedef struct {
  jint lineno;         // line number in the source file
  jmethodID methodId; // method executed in this frame
} ASGCTCallFrame;

typedef struct {
  JNIEnv *envId;          // Env where trace was recorded
  jint numFrames;         // number of frames in this trace
  ASGCTCallFrame *frames; // frames
} ASGCTCallTrace;

typedef void (*AsyncGetCallTrace)(ASGCTCallTrace *trace, jint depth, void *ucontext);
}
#endif