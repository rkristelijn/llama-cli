/**
 * @file test_markdown.cpp
 * @brief Integration tests: markdown renderer, on/off toggle
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>

#include "test_helpers.h"
#include "tui.h"

SCENARIO("Markdown rendering on vs off in REPL") {
  GIVEN("the LLM responds with markdown") {
    auto md_chat = [](const std::vector<Message>&) { return "# Title\n**bold** and *italic*\n- item one"; };

    WHEN("markdown is off via /set") {
      std::istringstream in("/set markdown\nrender\nexit\n");
      std::ostringstream out;
      run_repl(md_chat, test_cfg(), in, out);
      THEN("raw markdown is preserved") {
        CHECK(out.str().find("# Title") != std::string::npos);
        CHECK(out.str().find("**bold**") != std::string::npos);
      }
    }
  }
}

SCENARIO("Markdown renderer") {
  GIVEN("color is enabled") {
    WHEN("rendering a heading") {
      auto r = tui::render_markdown("# Hello", true);
      THEN("# is stripped") {
        CHECK(r.find("Hello") != std::string::npos);
        CHECK(r.find("# ") == std::string::npos);
      }
    }
    WHEN("rendering bold") {
      auto r = tui::render_markdown("**bold**", true);
      THEN("** is stripped") {
        CHECK(r.find("bold") != std::string::npos);
        CHECK(r.find("**") == std::string::npos);
      }
    }
    WHEN("rendering italic") {
      auto r = tui::render_markdown("*italic*", true);
      THEN("* is stripped") {
        CHECK(r.find("italic") != std::string::npos);
        CHECK(r.find("*italic*") == std::string::npos);
      }
    }
    WHEN("rendering inline code") {
      auto r = tui::render_markdown("`code`", true);
      THEN("backticks are stripped") {
        CHECK(r.find("code") != std::string::npos);
        CHECK(r.find("`code`") == std::string::npos);
      }
    }
    WHEN("rendering a bullet list") {
      auto r = tui::render_markdown("- item one\n- item two", true);
      THEN("bullets are converted") {
        CHECK(r.find("•") != std::string::npos);
        CHECK(r.find("item one") != std::string::npos);
      }
    }
    WHEN("rendering a numbered list") {
      auto r = tui::render_markdown("1. first\n2. second", true);
      THEN("numbers preserved") { CHECK(r.find("1.") != std::string::npos); }
    }
    WHEN("rendering a code block") {
      auto r = tui::render_markdown("```\nint x = 1;\n```", true);
      THEN("code preserved") { CHECK(r.find("int x = 1;") != std::string::npos); }
    }
  }
  GIVEN("color is disabled") {
    WHEN("rendering markdown") {
      auto r = tui::render_markdown("# Hello\n**bold**", false);
      THEN("text unchanged") { CHECK(r == "# Hello\n**bold**"); }
    }
  }
}
