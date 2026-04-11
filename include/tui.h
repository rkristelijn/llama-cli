/**
 * @file tui.h
 * @brief Terminal UI helpers — ANSI colors with TTY detection
 * @see docs/adr/adr-016-tui-design.md
 */
#ifndef TUI_H
#define TUI_H

#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace tui {

/** Check if color output is enabled (TTY + no NO_COLOR + no --no-color) */
inline bool use_color(bool no_color_flag = false) {
  if (no_color_flag) return false;
  if (std::getenv("NO_COLOR")) return false;
  return isatty(STDOUT_FILENO) != 0;
}

inline void prompt(std::ostream& out, bool color) { out << (color ? "\033[1;32m> \033[0m" : "> "); }

inline void system_msg(std::ostream& out, bool color, const std::string& msg) {
  out << (color ? "\033[2m" : "") << msg << (color ? "\033[0m" : "") << "\n";
}

inline void error(std::ostream& out, bool color, const std::string& msg) {
  out << (color ? "\033[1;31m" : "") << msg << (color ? "\033[0m" : "") << "\n";
}

inline void cmd_output(std::ostream& out, bool color, const std::string& msg) {
  out << (color ? "\033[36m" : "") << msg << (color ? "\033[0m" : "") << "\n";
}

inline void write_proposal(std::ostream& out, bool color, const std::string& path) {
  out << (color ? "\033[33m" : "") << "[proposed: write " << path << "]" << (color ? "\033[0m" : "") << "\n";
}

}  // namespace tui

#endif
