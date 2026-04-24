/**
 * @file logger_test.cpp
 * @brief Unit tests for logger module.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "logging/logger.h"

#include <doctest/doctest.h>

#include <cstdio>
#include <fstream>
#include <string>

SCENARIO ("Logger writes JSONL events") {
  GIVEN ("a logger instance") {
    Logger& logger = Logger::instance();
    THEN ("log path is set") {
      CHECK (!logger.path().empty())
        ;
      CHECK (logger.path().find("events.jsonl") != std::string::npos)
        ;
    }
    WHEN ("an event is logged") {
      Event e;
      e.agent = "test";
      e.action = "unit_test";
      e.input = "hello";
      e.output = "world";
      e.duration_ms = 42;
      e.tokens_prompt = 10;
      e.tokens_completion = 20;
      logger.log(e);
      THEN ("the log file contains the event") {
        std::ifstream f(logger.path());
        std::string last_line;
        std::string line;
        while (std::getline(f, line)) {
          last_line = line;
        }
        CHECK (last_line.find("\"agent\":\"test\"") != std::string::npos)
          ;
        CHECK (last_line.find("\"action\":\"unit_test\"") != std::string::npos)
          ;
        CHECK (last_line.find("\"duration_ms\":42") != std::string::npos)
          ;
      }
    }
    WHEN ("an event with special chars is logged") {
      Event e;
      e.agent = "test";
      e.action = "escape";
      e.input = "line1\nline2\ttab";
      e.output = "quote\"backslash\\";
      logger.log(e);
      THEN ("special chars are escaped") {
        std::ifstream f(logger.path());
        std::string last_line;
        std::string line;
        while (std::getline(f, line)) {
          last_line = line;
        }
        CHECK (last_line.find("\\n") != std::string::npos)
          ;
        CHECK (last_line.find("\\t") != std::string::npos)
          ;
        CHECK (last_line.find("\\\"") != std::string::npos)
          ;
        CHECK (last_line.find("\\\\") != std::string::npos)
          ;
      }
    }
    WHEN ("LOG_EVENT macro is used") {
      // The macro is the primary API for logging throughout the codebase.
      // Verify it writes the correct fields to the JSONL file.
      LOG_EVENT("repl", "session_start", "gemma4:e4b", "localhost:11434", 0, 0, 0);
      THEN ("the event appears in the log with correct fields") {
        std::ifstream f(logger.path());
        std::string last_line;
        std::string line;
        while (std::getline(f, line)) {
          last_line = line;
        }
        CHECK (last_line.find("\"agent\":\"repl\"") != std::string::npos)
          ;
        CHECK (last_line.find("\"action\":\"session_start\"") != std::string::npos)
          ;
        CHECK (last_line.find("\"input\":\"gemma4:e4b\"") != std::string::npos)
          ;
        CHECK (last_line.find("\"output\":\"localhost:11434\"") != std::string::npos)
          ;
      }
    }
    WHEN ("LOG_EVENT logs an exec with duration") {
      // Exec events carry timing data for performance analysis
      LOG_EVENT("repl", "exec", "ls -la", "file1\nfile2", 150, 0, 0);
      THEN ("duration_ms is recorded") {
        std::ifstream f(logger.path());
        std::string last_line;
        std::string line;
        while (std::getline(f, line)) {
          last_line = line;
        }
        CHECK (last_line.find("\"action\":\"exec\"") != std::string::npos)
          ;
        CHECK (last_line.find("\"duration_ms\":150") != std::string::npos)
          ;
      }
    }
    WHEN ("LOG_EVENT logs a file operation") {
      // File write/read events form an audit trail
      LOG_EVENT("repl", "file_write", "/tmp/test.txt", "ok", 0, 0, 0);
      LOG_EVENT("repl", "file_read", "/tmp/test.txt", "", 0, 0, 0);
      THEN ("both events are in the log") {
        std::ifstream f(logger.path());
        bool found_write = false;
        bool found_read = false;
        std::string line;
        while (std::getline(f, line)) {
          if (line.find("\"action\":\"file_write\"") != std::string::npos) {
            found_write = true;
          }
          if (line.find("\"action\":\"file_read\"") != std::string::npos) {
            found_read = true;
          }
        }
        CHECK (found_write)
          ;
        CHECK (found_read)
          ;
      }
    }
  }
}

// --- Logger path and edge cases ---

SCENARIO ("Logger path detection") {
  GIVEN ("the logger singleton") {
    WHEN ("path is queried") {
      const auto& logger = Logger::instance();
      THEN ("path is non-empty and ends with events.jsonl") {
        CHECK (!logger.path().empty())
          ;
        CHECK (logger.path().find("events.jsonl") != std::string::npos)
          ;
      }
    }
  }
}

SCENARIO ("Logger escapes control characters") {
  GIVEN ("a logger instance") {
    auto& logger = Logger::instance();
    WHEN ("an event with tabs and newlines is logged") {
      Event e;
      e.agent = "test";
      e.action = "escape";
      e.input = "line1\nline2";
      e.output = "tab\there";
      logger.log(e);
      THEN ("the log file contains escaped sequences") {
        std::ifstream f(logger.path());
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        CHECK (content.find("\\n") != std::string::npos)
          ;
        CHECK (content.find("\\t") != std::string::npos)
          ;
      }
    }
  }
}

// TODO: test Logger dev-mode vs installed-mode path detection
// TODO: test Logger with unwritable log path (permission denied)
// TODO: test Logger escape of all RFC 8259 control characters (\b, \f, \u00xx)
