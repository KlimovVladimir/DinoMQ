#include "log.h"

int debug = 6; //TODO

void logPrint(int debugLevel, const char *format, ...)
{
    if (debugLevel > debug)
        return;

    va_list arg;
    va_start (arg, format);
    vfprintf (stdout, format, arg);
    va_end (arg);
}