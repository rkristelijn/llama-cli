/**
 * @file logger.cpp
 * @brief Logger implementation.
 * @see logger.h
 */

#include "logging/logger.h"

#include <dirent.h>
#include <sys/stat.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>

Logger::Logger() {
  // Dev mode: if Makefile exists in cwd, we're running from the repo — log to .tmp/
  // Installed mode: log to ~/.llama-cli/ so user logs are separate from dev logs.
  struct stat st;
  if (stat("Makefile", &st) == 0) {
    mkdir(".tmp", 0755);
    log_path_ = ".tmp/events.jsonl";
  } else {
    const char* home = getenv("HOME");
    log_path_ = std::string(home ? home : ".") + "/.llama-cli/events.jsonl";
    std::string dir = log_path_.substr(0, log_path_.rfind('/'));
    mkdir(dir.c_str(), 0755);
  }
}

/// Singleton accessor — creates the logger on first call.
/// Path is auto-detected: .tmp/ for dev, ~/.llama-cli/ for installed.
Logger& Logger::instance() {
  static Logger instance;
  return instance;
}

/// Return the resolved log file path (for /log command and tests).
const std::string& Logger::path() const { return log_path_; }

/// Write a structured event to the JSONL log file.
/// Each event is one JSON line with timestamp, agent, action, I/O, and metrics.
/// File is opened in append mode — safe for concurrent writes.
void Logger::log(const Event& e) {
  // Get current timestamp in UTC
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  std::ostringstream ts;
  ts << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%S");
  ts << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';

  // Escape special characters for JSON (complete per RFC 8259).
  // Handles quotes, backslashes, control chars, and non-printable bytes.
  // Returns a string safe for embedding in a JSON string value.
  auto escape = [](const std::string& s) {
    std::string r;
    for (unsigned char c : s) {
      switch (c) {
        case '"':
          r += "\\\"";
          break;
        case '\\':
          r += "\\\\";
          break;
        case '\b':
          r += "\\b";
          break;
        case '\f':
          r += "\\f";
          break;
        case '\n':
          r += "\\n";
          break;
        case '\r':
          r += "\\r";
          break;
        case '\t':
          r += "\\t";
          break;
        default:
          if (c < 0x20) {
            char buf[7];
            snprintf(buf, sizeof(buf), "\\u%04x", c);
            r += buf;
          } else {
            r += static_cast<char>(c);
          }
      }
    }
    return r;
  };

  // Write JSONL entry
  std::ofstream out(log_path_, std::ios::app);
  if (out) {
    out << "{\"timestamp\":\"" << ts.str() << "\",";
    out << "\"agent\":\"" << escape(e.agent) << "\",";
    out << "\"action\":\"" << escape(e.action) << "\",";
    out << "\"input\":\"" << escape(e.input) << "\",";
    out << "\"output\":\"" << escape(e.output) << "\",";
    out << "\"duration_ms\":" << e.duration_ms << ",";
    out << "\"tokens_prompt\":" << e.tokens_prompt << ",";
    out << "\"tokens_completion\":" << e.tokens_completion << ",";
    out << "\"rating\":\"" << escape(e.rating) << "\"}\n";
  }
}
