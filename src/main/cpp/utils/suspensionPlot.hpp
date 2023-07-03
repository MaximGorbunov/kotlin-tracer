#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_SUSPENSIONPLOT_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_SUSPENSIONPLOT_H_
#include <memory>
#include "../profiler/model/trace.hpp"
#include "../trace/traceStorage.hpp"

namespace kotlin_tracer {
  void plot(const std::string &path, const TraceInfo &trace_info, const TraceStorage &storage);
}
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_SUSPENSIONPLOT_H_
