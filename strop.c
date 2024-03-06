#include "strop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

const char* format (const char *fmt, ...) {
  va_list arg;
  va_list cpy;
  
  va_start(arg, fmt);
  va_copy(cpy, arg);

  int size = vsnprintf(NULL, 0, fmt, cpy);
  va_end(cpy);

  char *buf = malloc(size+1);
  vsnprintf(buf, size+1, fmt, arg);
  va_end(arg);

  return buf;
}
void format_arg_to (const char* fmt, char* dest, int size, va_list args) {
  vsnprintf(dest, size, fmt, args);
  va_end(args);
}

void format_to (const char* fmt, char* dest, int size, ...) {
  va_list args;
  va_start(args, size);
  format_arg_to(fmt, dest, size, args);
}
