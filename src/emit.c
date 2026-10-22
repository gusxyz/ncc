#include <emit.h>
#include <stdarg.h>
#include <stdio.h>

void emit(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fputs("  ", stdout);
  vfprintf(stdout, fmt, args);
  fputc('\n', stdout);
  va_end(args);
}

void emit_label(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    fputs(":\n", stdout);
    va_end(args);
}

void emit_directive(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    fputc('\n', stdout);
    va_end(args);
}
