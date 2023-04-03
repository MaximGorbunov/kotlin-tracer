#include <jvmti.h>

#ifndef TRACE_H
#define TRACE_H

typedef struct {
  jint lineno;         // line number in the source file
  jmethodID method_id; // method executed in this frame
} ASGCT_CallFrame;

typedef struct {
  JNIEnv *env_id;          // Env where trace was recorded
  jint num_frames;         // number of frames in this trace
  ASGCT_CallFrame *frames; // frames
} ASGCT_CallTrace;

typedef void (*AsyncGetCallTrace)(ASGCT_CallTrace *trace, jint depth, void *ucontext);

#endif