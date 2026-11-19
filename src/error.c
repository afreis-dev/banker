#include <stdarg.h>
#include <stdio.h>

#include "error.h"

static void report(const char *label, const char *format, va_list arguments)
{
    fprintf(stderr, "banker: %s: ", label);
    vfprintf(stderr, format, arguments);
    fputc('\n', stderr);
}

void report_error(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    report("error", format, arguments);
    va_end(arguments);
}

void report_warning(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    report("warning", format, arguments);
    va_end(arguments);
}
