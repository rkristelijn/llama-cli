/**
 * @file trace.cpp
 * @brief Trace implementation.
 * @see trace.h
 */

#include "trace/trace.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>

StderrTrace stderr_trace_instance;  ///< Global instance for production use

/// Write trace message to stderr with local-time HH:MM:SS timestamp.
void StderrTrace::log(const char* fmt, ...) {
  // ... (timestamp code)
  // Print ISO 8601 timestamp with milliseconds (same format as event log)
  struct timespec ts_now;
  clock_gettime(CLOCK_REALTIME, &ts_now);
  struct tm tm_buf;
  gmtime_r(&ts_now.tv_sec, &tm_buf);
  char ts[30];
  strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", &tm_buf);
  fprintf(stderr, "[%s.%03ldZ] ", ts, ts_now.tv_nsec / 1000000);

  // Initialize variable argument list from the format string.
  // Note: clang-tidy sometimes reports false positives for uninitialized va_list
  // even when va_start is correctly called in virtual function overrides.
  va_list args;
  va_start(args, fmt);
  // NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized)
  vfprintf(stderr, fmt, args);
  va_end(args);
}

void CapturingTrace::log(const char* fmt, ...) {
  char buf[256];
  // Initialize variable argument list for in-memory capture.
  // Standard pattern: va_start -> vsnprintf -> va_end.
  va_list args;
  va_start(args, fmt);
  // NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized)
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  messages.push_back(buf);
}
