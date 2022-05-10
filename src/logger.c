#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define TIME_BUFFER_LEN 100

void __lprintf(const char *tag, const char *fmt, ...)
{
    // Get time
    time_t rawtime;
    struct tm info;
    time(&rawtime);
    localtime_r(&rawtime, &info);

    // Time to string
    char time_buffer[TIME_BUFFER_LEN];
    strftime(time_buffer, sizeof(time_buffer), "%x - %H:%M:%S %Z", &info);

    fprintf(LOG_STREAM, "[%s] %s: ", tag, time_buffer);

    va_list args;
    va_start(args, fmt);
    vfprintf(LOG_STREAM, fmt, args);
    va_end(args);
}

