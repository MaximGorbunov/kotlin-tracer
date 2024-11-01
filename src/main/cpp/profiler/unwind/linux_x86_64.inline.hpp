#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_LINUX_X86_64_INLINE_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_LINUX_X86_64_INLINE_H_

#if !defined(__APPLE__) && (defined(__x86_64__) || defined(_M_X64))
#define UNW_LOCAL_ONLY
#include <libunwind-x86_64.h>

#include "../../utils/log.h"
#include "vm/jvm.hpp"
#include "profiler/model/asyncTrace.hpp"

namespace kotlin_tracer {

static inline void unwind_stack(ucontext_t *ucontext, JVM *jvm, AsyncTrace *trace) {
  unw_context_t context;
  unw_cursor_t cursor;
  unw_getcontext(&context);
  unw_init_local(&cursor, &context);
  unw_word_t ip, rbp;

  //Skip signal frame
  while (unw_step(&cursor) > 0) {
    unw_get_reg(&cursor, UNW_REG_IP, &ip);
    unw_get_reg(&cursor, UNW_X86_64_RBP, &rbp);
    if (ucontext->uc_mcontext.gregs[REG_RIP] == static_cast<long long>(ip)) break;
  }

  int trace_index = 0;
  unw_word_t lrbp = 0;
  do {
    if (lrbp != 0 && lrbp < rbp) break;
    unw_get_reg(&cursor, UNW_REG_IP, &ip);
    unw_get_reg(&cursor, UNW_X86_64_RBP, &rbp);
    if (!jvm->isJavaFrame(ip)) {
//        abi::__cxa_demangle(buf,
      trace->instructions[trace_index].instruction = static_cast<intptr_t>(ip);
      trace->instructions[trace_index].frame = rbp;
      trace->instructions[trace_index].java_frame = false;
    } else {
      auto jmethod_id = jvm->getJMethodId(ip, rbp);
      trace->instructions[trace_index].instruction = reinterpret_cast<intptr_t>(jmethod_id);
      trace->instructions[trace_index].frame = 0;
      trace->instructions[trace_index].java_frame = true;
    };
    trace_index++;
    lrbp = rbp;
  } while(unw_step(&cursor) > 0 && trace_index < trace->size);
  trace->size = trace_index;
}  //kotlin_tracer
}
#endif //!defined(__APPLE__) && (defined(__x86_64__) || defined(_M_X64))
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_LINUX_X86_64_INLINE_H_
