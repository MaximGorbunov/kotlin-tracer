#ifndef KOTLIN_TRACER_LOG_H
#define KOTLIN_TRACER_LOG_H

#include <string>
#include <iostream>

#define logInfo(msg) std::cout << (msg) << std::endl

#ifdef DEBUG
#define logDebug(msg) std::cout << (msg) << std::endl
#else
#define log_debug(msg)
#endif

#endif //KOTLIN_TRACER_LOG_H
