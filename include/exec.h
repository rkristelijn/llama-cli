// exec.h — Shell command execution with timeout and output capture
// Used by ! (direct), !! (context), and <exec> (LLM-proposed)

#ifndef EXEC_H
#define EXEC_H

#include <string>

/// Result of executing a shell command
struct ExecResult {
  std::string output;  ///< Captured stdout+stderr
  int exit_code;       ///< Process exit code (or -1 on error)
  bool timed_out;      ///< True if killed by timeout
};

// Execute a shell command, capturing output
// Merges stderr into stdout, enforces timeout, truncates at max_chars
ExecResult cmd_exec(const std::string& command, int timeout_secs, int max_chars);

#endif
