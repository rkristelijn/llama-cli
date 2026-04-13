/**
 * @file trace.h
 * @brief Trace interface for debug output with dependency injection.
 *
 * Why dependency injection?
 * - Tests can inject a mock to capture trace output without cluttering stderr
 * - Production can use stderr or file output
 * - Easy to extend (e.g., JSON logging, network logging) without changing callers
 *
 * Usage:
 *   ReplState state;
 *   state.trace = \&stderr_trace;  // production
 *   state.trace = \&mock_trace;    // tests
 *
 *   if (state.trace) {
 *       state.trace->log("[TRACE] iteration=%d\n", count);
 *   }
 *
 * @see docs/adr/adr-028-execution-limits.md — trace mode for debugging loops
 */

#ifndef TRACE_H
#define TRACE_H

#include <cstdarg>
#include <string>
#include <vector>

/**
 * @brief Abstract trace interface for debug output.
 *
 * Implement this interface to provide custom trace behavior:
 * - StderrTrace: write to stderr (production)
 * - CapturingTrace: capture in memory (testing)
 * - FileTrace: write to a log file (future)
 */
class Trace {
 public:
  virtual ~Trace() = default;

  /**
   * @brief Write a formatted trace message.
   * @param fmt printf-style format string
   * @param ... format arguments
   */
  virtual void log(const char* fmt, ...) = 0;
};

/**
 * @brief Trace that writes to stderr.
 * Used in production when trace mode is enabled.
 */
class StderrTrace : public Trace {
 public:
  void log(const char* fmt, ...) override;
};

/**
 * @brief Trace that captures messages in memory.
 * Used in tests to verify trace output without stderr pollution.
 */
class CapturingTrace : public Trace {
 public:
  std::vector<std::string> messages;  ///< Captured trace messages

  void log(const char* fmt, ...) override;
};

/**
 * @brief Global stderr trace instance for production use.
 * Declared extern, defined in trace.cpp.
 */
extern StderrTrace stderr_trace_instance;

/**
 * @brief Pointer to global stderr trace for production use.
 * Points to stderr_trace_instance defined in trace.cpp.
 */
inline Trace* stderr_trace = &stderr_trace_instance;

#endif  // TRACE_H
