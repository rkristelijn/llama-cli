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
  }
}
