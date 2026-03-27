#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_APPLE_AARCH64_INLINE_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_APPLE_AARCH64_INLINE_H_
#if defined(__APPLE__) && defined(__aarch64__)

#include <ptrauth.h>

#include "../../utils/log.h"
#include "profiler/model/asyncTrace.hpp"
#include "vm/jvm.hpp"

namespace kotlin_tracer {

using std::string;

#define REGISTER(reg) uc_mcontext->__ss.__##reg
#define REGISTER_NEON(reg) uc_mcontext->__ns.__##reg

#define POINTER_ALIGNMENT sizeof(void*)

static inline uintptr_t strip_pointer(uintptr_t ptr) {
  return ptrauth_strip(ptr, ptrauth_key_asib);
}

static inline bool is_pointer_in_stack(uint64_t ptr, uint64_t lo, uint64_t hi) {
  return ptr >= lo && ptr < hi;
}


static inline void unwind_stack(ucontext_t* ucontext, JVM* jvm, AsyncTrace* trace,
                                                uint64_t stack_lo, uint64_t stack_hi) {
  uint64_t ip, fp, lr;
  ip = ucontext->REGISTER(pc);
  fp = ucontext->REGISTER(fp);
  lr = ucontext->REGISTER(lr);
  int trace_index = 0;
  bool processed = false;
  if (!is_pointer_in_stack(fp, stack_lo, stack_hi) || fp % POINTER_ALIGNMENT != 0 || fp == 0 ||
      ip == 0) {
    trace->size = 0;
    return;
  }
  do {
    processed = false;
    if (!jvm->isJavaFrame(ip)) {
      trace->instructions[trace_index].instruction = static_cast<intptr_t>(ip);
      trace->instructions[trace_index].frame = fp;
      trace->instructions[trace_index].java_frame = false;
      processed = true;
    } else {
      auto jmethodId = jvm->getJMethodId(ip, fp);
      trace->instructions[trace_index].instruction = reinterpret_cast<intptr_t>(jmethodId);
      trace->instructions[trace_index].frame = 0;
      trace->instructions[trace_index].java_frame = true;
      processed = jmethodId != nullptr;
    }
    // Stack frame
    // |-------------------------|
    // |    LR''                 | FP' + 8
    // |-------------------------|
    // |    FP''                 |
    // |-------------------------| <-
    // |    ..                   |  |
    // |-------------------------|  |
    // |    LR'                  |  |
    // |-------------------------"  |
    // |    FP'                  | -|  <- FP
    // |-------------------------|
    // |    Stack args           |
    // |_________________________|
    trace_index++;
    auto last_fp = fp;
    fp = *reinterpret_cast<uint64_t*>(strip_pointer(fp));
    if (!is_pointer_in_stack(fp + 8, stack_lo, stack_hi) || fp % POINTER_ALIGNMENT != 0 ||
        fp < last_fp || fp == 0 || lr == 0)
      break;
    ip = strip_pointer(lr);
    lr = *reinterpret_cast<uint64_t*>(strip_pointer(fp + 8));
  } while (processed && trace_index < trace->size);
  trace->size = trace_index;
}
}  // namespace kotlin_tracer
#endif  // defined(__APPLE__) && defined(__ARM_ARCH)
#endif  // KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_APPLE_AARCH64_INLINE_H_
