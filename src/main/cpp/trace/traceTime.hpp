#ifndef TRACE_TIME_H
#define TRACE_TIME_H

#include <sys/time.h>
#include <chrono>

namespace kotlinTracer {
typedef long long TraceTime;

TraceTime currentTimeMs();
TraceTime currentTimeNs();
}
#endif
