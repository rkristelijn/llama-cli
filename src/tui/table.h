/**
 * @file table.h
 * @brief Markdown table parsing and rendering with terminal-width awareness.
 *
 * SRP: All table-related logic lives here — parsing, width calculation, rendering.
 * Extracted from markdown.cpp to reduce file size (ADR-061, ADR-065).
 *
 * @see docs/adr/adr-052-markdown-renderer.md
 */

#ifndef TUI_TABLE_H
#define TUI_TABLE_H

#include <string>
#include <vector>

namespace tui {

/// Check if a line is a markdown table row (starts with |).
bool is_table_row(const std::string& line);

/// Check if a line is a table separator (| --- | --- |).
bool is_table_separator(const std::string& line);

/// Split a table row into trimmed cell strings (pipe-delimited).
std::vector<std::string> parse_table_cells(const std::string& line);

/// Compute visible text length after markdown markers are stripped.
size_t visible_length(const std::string& text);

/// Render a collected table block with terminal-width-aware column padding.
std::string render_table(const std::vector<std::string>& rows, bool color);

/// Get terminal width in columns via ioctl. Falls back to 80.
int get_terminal_width();

}  // namespace tui

#endif
