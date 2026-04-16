/**
 * @file trace.cpp
 * @brief Trace implementation.
 * @see trace.h
 */

#include "trace/trace.h"

#include <cstdio>

StderrTrace stderr_trace_instance;  ///< Global instance for production use

/// Write trace message to stderr.
/// First clears the current terminal line (\\r = move cursor to start,
/// \033[K = erase to end of line) so trace output doesn't collide
/// with the spinner animation that also writes to stderr.
void StderrTrace::log(const char* fmt, ...) {
  fprintf(stderr, "\r\033[K");
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

void CapturingTrace::log(const char* fmt, ...) {
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  messages.push_back(buf);
}
