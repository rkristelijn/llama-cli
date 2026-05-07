/**
 * @file table_test.cpp
 * @brief Unit tests for markdown table parsing utilities (ADR-052).
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "tui/table.h"

#include <doctest/doctest.h>

// --- Pure function tests: no terminal, no color ---

SCENARIO ("table: is_table_row detects pipe-prefixed lines") {
  CHECK (tui::is_table_row("| a | b |"))
    ;
  CHECK (tui::is_table_row("|single"))
    ;
  CHECK_FALSE (tui::is_table_row("no pipe"))
    ;
  CHECK_FALSE (tui::is_table_row(""))
    ;
}

SCENARIO ("table: is_table_separator identifies separator rows") {
  CHECK (tui::is_table_separator("| --- | --- |"))
    ;
  CHECK (tui::is_table_separator("|---|---|"))
    ;
  CHECK (tui::is_table_separator("| :---: | ---: |"))
    ;
  CHECK_FALSE (tui::is_table_separator("| text | here |"))
    ;
  CHECK_FALSE (tui::is_table_separator("not a table"))
    ;
  CHECK_FALSE (tui::is_table_separator(""))
    ;
}

SCENARIO ("table: parse_table_cells splits pipe-delimited content") {
  GIVEN ("a standard table row") {
    auto cells = tui::parse_table_cells("| hello | world |");
    CHECK (cells.size() == 2)
      ;
    CHECK (cells[0] == "hello")
      ;
    CHECK (cells[1] == "world")
      ;
  }
  GIVEN ("a row with extra whitespace") {
    auto cells = tui::parse_table_cells("|  padded  |  text  |");
    CHECK (cells.size() == 2)
      ;
    CHECK (cells[0] == "padded")
      ;
    CHECK (cells[1] == "text")
      ;
  }
  GIVEN ("a row with empty cells") {
    auto cells = tui::parse_table_cells("| | content | |");
    CHECK (cells.size() == 3)
      ;
    CHECK (cells[0] == "")
      ;
    CHECK (cells[1] == "content")
      ;
    CHECK (cells[2] == "")
      ;
  }
}

SCENARIO ("table: visible_length strips markdown markers") {
  // Plain text — no markers
  CHECK (tui::visible_length("hello") == 5)
    ;
  // Bold markers removed, inner text counted
  CHECK (tui::visible_length("**bold**") == 4)
    ;
  // Italic markers removed
  CHECK (tui::visible_length("*italic*") == 6)
    ;
  // Code backticks removed
  CHECK (tui::visible_length("`code`") == 4)
    ;
  // Strikethrough markers removed
  CHECK (tui::visible_length("~~strike~~") == 6)
    ;
  // Mixed: plain + bold — "a **b** c" → "a b c" = 5 chars
  CHECK (tui::visible_length("a **b** c") == 5)
    ;
}

SCENARIO ("table: render_table produces output for valid rows") {
  std::vector<std::string> rows = {"| A | B |", "| --- | --- |", "| 1 | 2 |"};
  // Render without color — pure text output
  std::string result = tui::render_table(rows, false);
  CHECK (result.find("A") != std::string::npos)
    ;
  CHECK (result.find("1") != std::string::npos)
    ;
  CHECK (result.find("---") != std::string::npos)
    ;
}

SCENARIO ("table: render_table handles empty input") {
  std::vector<std::string> empty;
  CHECK (tui::render_table(empty, false) == "")
    ;
}
