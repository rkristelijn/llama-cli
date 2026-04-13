/**
 * @file trace.cpp
 * @brief Trace implementation.
 * @see trace.h
 */

#include "trace/trace.h"

#include <cstdio>

StderrTrace stderr_trace_instance;  ///< Global instance for production use

void StderrTrace::log(const char* fmt, ...) {
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
