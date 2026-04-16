/**
 * @file trace.cpp
 * @brief Trace implementation.
 * @see trace.h
 */

#include "trace/trace.h"

#include <cstdio>
#include <ctime>

StderrTrace stderr_trace_instance;  ///< Global instance for production use

/// Write trace message to stderr with local-time HH:MM:SS timestamp.
void StderrTrace::log(const char* fmt, ...) {
  fprintf(stderr, "\r\033[K");
  // Print timestamp
  time_t now = time(nullptr);
  struct tm tm_buf;
  localtime_r(&now, &tm_buf);
  char ts[20];
  strftime(ts, sizeof(ts), "%H:%M:%S", &tm_buf);
  fprintf(stderr, "[%s] ", ts);
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
