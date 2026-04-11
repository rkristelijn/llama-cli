/**
 * @file exec.cpp
 * @brief Shell command execution with timeout and output capture.
 * @see docs/adr/adr-015-command-execution.md
 */

#include "exec.h"

#include <sys/wait.h>

#include <cstdio>
#include <ctime>
#include <string>

/** Append buffer to output, truncating at max_chars if needed
 * Resizes output and appends a truncation marker when limit is hit
 * Returns true if output was truncated */
static bool append_output(std::string& output, const char* buf, int max_chars) {
  output += buf;
  if (static_cast<int>(output.size()) > max_chars) {
    output.resize(max_chars);
    output += "\n[truncated at " + std::to_string(max_chars) + " chars]";
    return true;
  }
  return false;
}

/** Execute a shell command, capturing stdout and stderr
 * Merges stderr into stdout via 2>&1 redirect
 * Enforces wall-clock timeout and truncates output at max_chars */
ExecResult cmd_exec(const std::string& command, int timeout_secs, int max_chars) {
  ExecResult result = {"", -1, false};
  // Merge stderr into stdout so we capture all output
  std::string cmd = command + " 2>&1";

  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    result.output = "Error: could not execute command";
    return result;
  }

  // Read output line by line, checking timeout and max_chars
  time_t start = time(nullptr);
  char buf[256];
  bool truncated = false;
  while (fgets(buf, sizeof(buf), pipe) != nullptr) {
    // Check wall-clock timeout on each line read
    if (time(nullptr) - start >= timeout_secs) {
      result.timed_out = true;
      break;
    }
    if (!truncated) {
      truncated = append_output(result.output, buf, max_chars);
    }
  }

  int status = pclose(pipe);
  if (result.timed_out) {
    result.output += "\n[killed: timeout after " + std::to_string(timeout_secs) + "s]";
    result.exit_code = -1;
  } else {
    result.exit_code = WEXITSTATUS(status);
  }
  return result;
}
