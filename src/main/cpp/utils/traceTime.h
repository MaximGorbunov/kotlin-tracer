#ifndef TRACE_TIME_H
#define TRACE_TIME_H

#include <sys/time.h>
#include <chrono>


typedef long long trace_time;

trace_time current_time_ms();
trace_time current_time_ns();

#endif
