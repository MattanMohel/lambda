#ifndef __STROP_H__
#define __STROP_H__

#include <stdarg.h>

const char* format (const char* fmt, ...);
void format_arg_to (const char* fmt, char* dest, int size, va_list args);
void format_to (const char* fmt, char* dest, int size, ...);

#endif
