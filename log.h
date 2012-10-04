#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include "config.h"

#ifdef LOGGING_ENABLED
#define Log(s) log_string(s)
#else
#define Log(s)
#endif

void log_string(const char *str);

#endif

