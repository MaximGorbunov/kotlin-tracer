#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_APPLE_X86_64_INLINE_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_APPLE_X86_64_INLINE_H_
#if defined(__APPLE__) && defined(__x86_64__)

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <string>

#include "../../utils/log.h"
#include "vm/jvm.hpp"
#include "profiler/model/asyncTrace.hpp"

namespace kotlin_tracer {

using std::string;

#define REGISTER(reg) uc_mcontext->__ss.__##reg

static inline void recover_frame(unw_cursor_t &cursor) {
  unw_word_t rbx;
  unw_get_reg(&cursor, UNW_X86_64_RBX, &rbx);
  auto *ucontext = (ucontext_t *) rbx;
  unw_set_reg(&cursor, UNW_X86_64_RAX, ucontext->REGISTER(rax));
  unw_set_reg(&cursor, UNW_X86_64_RDX, ucontext->REGISTER(rdx));
  unw_set_reg(&cursor, UNW_X86_64_RCX, ucontext->REGISTER(rcx));
  unw_set_reg(&cursor, UNW_X86_64_RBX, ucontext->REGISTER(rbx));
  unw_set_reg(&cursor, UNW_X86_64_RSI, ucontext->REGISTER(rsi));
  unw_set_reg(&cursor, UNW_X86_64_RDI, ucontext->REGISTER(rdi));
  unw_set_reg(&cursor, UNW_X86_64_RBP, ucontext->REGISTER(rbp));
  unw_set_reg(&cursor, UNW_X86_64_R8, ucontext->REGISTER(r8));
  unw_set_reg(&cursor, UNW_X86_64_R9, ucontext->REGISTER(r9));
  unw_set_reg(&cursor, UNW_X86_64_R10, ucontext->REGISTER(r10));
  unw_set_reg(&cursor, UNW_X86_64_R11, ucontext->REGISTER(r11));
  unw_set_reg(&cursor, UNW_X86_64_R12, ucontext->REGISTER(r12));
  unw_set_reg(&cursor, UNW_X86_64_R13, ucontext->REGISTER(r13));
  unw_set_reg(&cursor, UNW_X86_64_R14, ucontext->REGISTER(r14));
  unw_set_reg(&cursor, UNW_X86_64_R15, ucontext->REGISTER(r15));
  unw_set_reg(&cursor, UNW_REG_IP, ucontext->REGISTER(rip));
  unw_set_reg(&cursor, UNW_X86_64_RSP, ucontext->REGISTER(rsp));
}

static inline void unwind_stack(ucontext_t *ucontext, const std::shared_ptr<JVM> &jvm, AsyncTrace *trace) {
  unw_context_t context;
  unw_cursor_t cursor;
  unw_getcontext(&context);
  unw_init_local(&cursor, &context);
  unw_word_t ip, rbp, offset;
  char buf[256];
  //Skip until _sigtramp
  while (unw_step(&cursor) > 0) {
    if (!unw_get_proc_name(&cursor, buf, sizeof(buf), &offset)) {
      auto name = string(buf);
      if (name == "_sigtramp") {
        // At x86_64 macos sets pointer to ucontext in RBX register
        //  recover_frame(cursor);
        recover_frame(cursor);
        break;
      }
    }
  };
  unw_get_reg(&cursor, UNW_REG_IP, &ip);
  unw_get_reg(&cursor, UNW_X86_64_RBP, &rbp);
  Dl_info info{};
  bool processed = false;
  int trace_index = 0;

  do {
    processed = false;
    if (dladdr(reinterpret_cast<void *>(ip), &info)) {
      trace->instructions[trace_index].instruction = static_cast<intptr_t>(ip);
      trace->instructions[trace_index].frame = rbp;
      trace->instructions[trace_index].java_frame = false;
      processed = true;
    } else {
      auto jmethodId = jvm->getJMethodId(ip, rbp);
      trace->instructions[trace_index].instruction = reinterpret_cast<intptr_t>(jmethodId);
      trace->instructions[trace_index].frame = 0;
      trace->instructions[trace_index].java_frame = true;
      processed = jmethodId != nullptr;
    }
    //         Stack frame
    // |-------------------------|
    // |    return address       | RBP + 8
    // |-------------------------|
    // |    saved rbp            |
    // |_________________________|
    auto last_ip = ip;
    ip = *reinterpret_cast<uint64_t *>(rbp + 8);
    auto last_rbp = rbp;
    rbp = *reinterpret_cast<uint64_t *>(rbp);
    if (rbp < last_rbp || last_ip == ip || rbp == 0) break;
    trace_index++;
  } while (processed && trace_index < trace->size);
  trace->size = trace_index;
}
}  // kotlin_tracer
#endif  // defined(__APPLE__) && defined(__aarch64__)
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_APPLE_X86_64_INLINE_H_
