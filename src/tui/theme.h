/**
 * @file theme.h
 * @brief Color theme system — all styling goes through Theme, never hardcoded.
 *
 * Each role has a ThemeStyle (fg color, bg color, bold, italic, underline).
 * Built-in themes: dark (default), light, mono, hacker.
 * Custom: /theme set prompt green bold underline
 */

#ifndef THEME_H
#define THEME_H

#include <string>

/// A single style — combines foreground, background, and attributes.
/// Named ThemeStyle to avoid conflict with macOS MacTypes.h typedef.
struct ThemeStyle {
  std::string fg;          ///< Foreground: "red","green","blue","cyan","yellow","magenta","white","black","default"
  std::string bg;          ///< Background: same options, or empty for none
  bool bold = false;       ///< Bold text
  bool italic = false;     ///< Italic text
  bool underline = false;  ///< Underlined text
  bool dim = false;        ///< Dim/faint text

  /// Render this style as an ANSI escape sequence. Empty string if no styling.
  std::string ansi() const {
    if (fg.empty() && bg.empty() && !bold && !italic && !underline && !dim) {
      return "";
    }
    std::string seq = "\033[";
    bool first = true;
    auto add = [&](const std::string& code) {
      if (!first) {
        seq += ";";
      }
      seq += code;
      first = false;
    };
    if (bold) {
      add("1");
    }
    if (dim) {
      add("2");
    }
    if (italic) {
      add("3");
    }
    if (underline) {
      add("4");
    }
    if (!fg.empty()) {
      add(fg_code(fg));
    }
    if (!bg.empty()) {
      add(bg_code(bg));
    }
    seq += "m";
    return seq;
  }

  /// Reset sequence
  static std::string reset() { return "\033[0m"; }

 private:
  /// Map color name to ANSI code with given base offset (30=fg, 40=bg).
  static std::string color_code(const std::string& name, int base, int bright_base) {
    // Standard colors
    if (name == "black") {
      return std::to_string(base);
    }
    if (name == "red") {
      return std::to_string(base + 1);
    }
    if (name == "green") {
      return std::to_string(base + 2);
    }
    if (name == "yellow") {
      return std::to_string(base + 3);
    }
    if (name == "blue") {
      return std::to_string(base + 4);
    }
    if (name == "magenta") {
      return std::to_string(base + 5);
    }
    if (name == "cyan") {
      return std::to_string(base + 6);
    }
    if (name == "white") {
      return std::to_string(base + 7);
    }
    // Bright/high-intensity colors
    if (name == "bright_black") {
      return std::to_string(bright_base);
    }
    if (name == "bright_red") {
      return std::to_string(bright_base + 1);
    }
    if (name == "bright_green") {
      return std::to_string(bright_base + 2);
    }
    if (name == "bright_yellow") {
      return std::to_string(bright_base + 3);
    }
    if (name == "bright_blue") {
      return std::to_string(bright_base + 4);
    }
    if (name == "bright_magenta") {
      return std::to_string(bright_base + 5);
    }
    if (name == "bright_cyan") {
      return std::to_string(bright_base + 6);
    }
    if (name == "bright_white") {
      return std::to_string(bright_base + 7);
    }
    return "";
  }

  /// Map color name to ANSI foreground code (30-37 standard, 90-97 bright).
  static std::string fg_code(const std::string& name) {
    std::string code = color_code(name, 30, 90);
    return code.empty() ? "39" : code;  // default foreground
  }

  /// Map color name to ANSI background code (40-47 standard, 100-107 bright).
  static std::string bg_code(const std::string& name) { return color_code(name, 40, 100); }
};

/// All color roles used by the TUI layer.
/// Each theme defines styles for these roles — see ADR-080.
struct Theme {
  std::string name;    ///< Theme identifier
  ThemeStyle prompt;   ///< User prompt (nick>)
  ThemeStyle ai;       ///< AI response text
  ThemeStyle system;   ///< System messages
  ThemeStyle error;    ///< Error messages
  ThemeStyle info;     ///< Info/command output
  ThemeStyle warning;  ///< Warnings/proposals
  ThemeStyle banner;   ///< Startup banner
  ThemeStyle code;     ///< Inline code
};

/// Built-in dark theme (default)
inline Theme theme_dark() {
  return {"dark",
          {"green", "", true, false, false, false},    // prompt: bold green
          {"", "", false, false, false, false},        // ai: default
          {"", "", false, false, false, true},         // system: dim
          {"red", "", true, false, false, false},      // error: bold red
          {"cyan", "", false, false, false, false},    // info: cyan
          {"yellow", "", false, false, false, false},  // warning: yellow
          {"yellow", "", true, false, false, false},   // banner: bold yellow
          {"cyan", "", false, false, false, false}};   // code: cyan
}

/// Light theme — blue prompt, gray AI
inline Theme theme_light() {
  return {"light",
          {"blue", "", true, false, false, false},           // prompt: bold blue
          {"bright_black", "", false, false, false, false},  // ai: gray
          {"", "", false, false, false, true},               // system: dim
          {"red", "", true, false, false, false},            // error: bold red
          {"blue", "", false, false, false, false},          // info: blue
          {"yellow", "", false, false, false, false},        // warning: yellow
          {"blue", "", true, false, false, false},           // banner: bold blue
          {"blue", "", false, false, false, false}};         // code: blue
}

/// Mono theme — no colors
inline Theme theme_mono() {
  // clang-format off
  return {"mono", {}, {}, {}, {}, {}, {}, {}, {}};
  // clang-format on
}

/// Hacker theme — all green
inline Theme theme_hacker() {
  return {"hacker",
          {"bright_green", "", true, false, false, false},    // prompt
          {"green", "", false, false, false, false},          // ai
          {"green", "", false, false, false, true},           // system
          {"red", "", true, false, false, false},             // error
          {"bright_green", "", false, false, false, false},   // info
          {"bright_yellow", "", false, false, false, false},  // warning
          {"bright_green", "", true, false, false, false},    // banner
          {"bright_green", "", false, false, false, false}};  // code
}

/// Get a theme by name. Returns dark for unknown names.
inline Theme get_theme(const std::string& name) {
  if (name == "light") {
    return theme_light();
  }
  if (name == "mono") {
    return theme_mono();
  }
  if (name == "hacker") {
    return theme_hacker();
  }
  return theme_dark();
}

#endif  // THEME_H
