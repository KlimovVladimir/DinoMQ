#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

enum { 
    LOG_EMERG,
    LOG_ALERT,
    LOG_CRIT,
    LOG_ERR,
    LOG_WARNING,
    LOG_NOTICE,
    LOG_INFO,
    LOG_DEBUG
};

void logPrint(int debugLevel, const char *format, ...);

#endif //LOG_H