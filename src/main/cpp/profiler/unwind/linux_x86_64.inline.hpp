#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_LINUX_X86_64_INLINE_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_LINUX_X86_64_INLINE_H_

#if !defined(__APPLE__) && (defined(__x86_64__) || defined(_M_X64))
#include <string>

#define UNW_LOCAL_ONLY
#include <libunwind-x86_64.h>

namespace kotlin_tracer {
static inline void unwind_stack() {
  unw_context_t context;
  unw_cursor_t cursor;
  unw_getcontext(&context);
  unw_init_local(&cursor, &context);
  unw_word_t ip, rbp, offset;
  char buf[256];
  bool initial_signal_skipped = false;
  while (unw_step(&cursor) > 0) {
    if (!initial_signal_skipped && !unw_is_signal_frame(&cursor)) {
      initial_signal_skipped = true;
      unw_get_reg(&cursor, UNW_REG_IP, &ip);
      unw_get_reg(&cursor, UNW_X86_64_RBP, &rbp);
      Dl_info info{};
      if (!unw_get_proc_name(&cursor, buf, sizeof(buf), &offset)) {
//        abi::__cxa_demangle(buf,
        if (dladdr(reinterpret_cast<void *>(ip), &info)) {
          auto name = string(buf);
          logDebug(string(info.dli_fname) + ":" + name);
        } else {
          logDebug(string(buf));
        }
      } else {
        logDebug("Symbol not found through dladdr");
        jvm_->getCodeCache(ip, rbp);
      };
    }
  }
}  //kotlin_tracer
}
#endif //!defined(__APPLE__) && (defined(__x86_64__) || defined(_M_X64))
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_PROFILER_UNWIND_LINUX_X86_64_INLINE_H_
