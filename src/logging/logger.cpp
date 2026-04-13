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
  const char* home = getenv("HOME");
  log_path_ = std::string(home ? home : ".") + "/.llama-cli/events.jsonl";

  // Ensure directory exists
  std::string dir = log_path_.substr(0, log_path_.rfind('/'));
  mkdir(dir.c_str(), 0755);
}

Logger& Logger::instance() {
  static Logger instance;
  return instance;
}

std::string Logger::path() const { return log_path_; }

void Logger::log(const Event& e) {
  // Get current timestamp
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  std::ostringstream ts;
  ts << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S") << '.' << std::setfill('0') << std::setw(3)
     << ms.count() << 'Z';

  // Escape special characters for JSON
  auto escape = [](const std::string& s) {
    std::string r;
    for (char c : s) {
      if (c == '"') {
        r += "\\\"";
      } else if (c == '\\') {
        r += "\\\\";
      } else if (c == '\n') {
        r += "\\n";
      } else if (c == '\r') {
        r += "\\r";
      } else if (c == '\t') {
        r += "\\t";
      } else {
        r += c;
      }
    }
    return r;
  };

  // Write JSONL entry
  std::ofstream out(log_path_, std::ios::app);
  if (out) {
    out << "{\"timestamp\":\"" << ts.str() << "\","
        << "\"agent\":\"" << escape(e.agent) << "\","
        << "\"action\":\"" << escape(e.action) << "\","
        << "\"input\":\"" << escape(e.input) << "\","
        << "\"output\":\"" << escape(e.output) << "\","
        << "\"duration_ms\":" << e.duration_ms << ","
        << "\"tokens_prompt\":" << e.tokens_prompt << ","
        << "\"tokens_completion\":" << e.tokens_completion << "}\n";
  }
}
