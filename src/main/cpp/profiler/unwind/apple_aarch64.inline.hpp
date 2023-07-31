#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_APPLE_AARCH64_INLINE_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_APPLE_AARCH64_INLINE_H_
#if defined(__APPLE__) && defined(__aarch64__)

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <string>
#include <ptrauth.h>

#include "../../utils/log.h"
#include "vm/jvm.hpp"
#include "profiler/model/asyncTrace.hpp"

namespace kotlin_tracer {

using std::string;

#define REGISTER(reg) uc_mcontext->__ss.__##reg
#define REGISTER_NEON(reg) uc_mcontext->__ns.__##reg

static inline uintptr_t strip_pointer(uintptr_t ptr) {
  return ptrauth_strip(ptr, ptrauth_key_asib);
}

[[maybe_unused]]
static inline void unwind_stack(ucontext_t *ucontext, const std::shared_ptr<JVM> &jvm, AsyncTrace *trace) {
  uint64_t ip, fp, lr;
  ip = ucontext->REGISTER(pc);
  fp = ucontext->REGISTER(fp);
  lr = ucontext->REGISTER(lr);
  Dl_info info{};
  int trace_index = 0;
  bool processed = false;
  do {
    processed = false;
    if (dladdr(reinterpret_cast<void *>(ip), &info)) {
      trace->instructions[trace_index].instruction = static_cast<intptr_t>(ip);
      trace->instructions[trace_index].frame = fp;
      trace->instructions[trace_index].java_frame = false;
      processed = true;
    } else {
      auto jmethodId = jvm->getCodeCache(ip, fp);
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
    auto last_ip = fp;
    fp = *reinterpret_cast<uint64_t *>(strip_pointer(fp));
    if (fp < last_fp || fp == 0 || last_ip == ip) break;
    ip = strip_pointer(lr);
    lr = *reinterpret_cast<uint64_t *>(strip_pointer(fp + 8));
  } while (processed && trace_index < trace->size);
  trace->size =trace_index;
}
}
#endif //defined(__APPLE__) && defined(__ARM_ARCH)
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_APPLE_AARCH64_INLINE_H_
