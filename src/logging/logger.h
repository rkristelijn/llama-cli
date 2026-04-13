/**
 * @file logger.h
 * @brief Structured event logging to JSONL file.
 * @see docs/adr/adr-027-event-logging.md
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <string>

/// Event data for structured logging
struct Event {
  std::string timestamp;      ///< ISO 8601 timestamp
  std::string agent;          ///< Agent name (e.g., "planner", "executor")
  std::string action;         ///< Action type (e.g., "exec", "llm_call")
  std::string input;          ///< Input/command/prompt
  std::string output;         ///< Output/response
  int duration_ms = 0;        ///< Execution time in milliseconds
  int tokens_prompt = 0;      ///< Prompt tokens
  int tokens_completion = 0;  ///< Completion tokens
};

/// Append-only JSONL logger for agent events
class Logger {
 public:
  /// Get singleton instance
  static Logger& instance();

  /// Log an event to ~/.llama-cli/events.jsonl
  void log(const Event& e);

  /// Get log file path
  std::string path() const;

 private:
  Logger();
  std::string log_path_;
};

/// Convenience macro for logging
#define LOG_EVENT(_agent_, _action_, _inp_, _out_, _dur_, _tp_, _tc_) \
  do {                                                                \
    Event e;                                                          \
    e.agent = _agent_;                                                \
    e.action = _action_;                                              \
    e.input = _inp_;                                                  \
    e.output = _out_;                                                 \
    e.duration_ms = _dur_;                                            \
    e.tokens_prompt = _tp_;                                           \
    e.tokens_completion = _tc_;                                       \
    Logger::instance().log(e);                                        \
  } while (0)

#endif  // LOGGER_H
