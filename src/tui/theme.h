/**
 * @file theme.h
 * @brief Color theme system — all styling goes through Theme, never hardcoded.
 *
 * Each role has a Style (fg color, bg color, bold, italic, underline).
 * Built-in themes: dark (default), light, mono, hacker.
 * Custom: /theme set prompt green bold underline
 */

#ifndef THEME_H
#define THEME_H

#include <string>

/// A single style — combines foreground, background, and attributes.
struct Style {
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
  static std::string fg_code(const std::string& name) {
    if (name == "black") {
      return "30";
    }
    if (name == "red") {
      return "31";
    }
    if (name == "green") {
      return "32";
    }
    if (name == "yellow") {
      return "33";
    }
    if (name == "blue") {
      return "34";
    }
    if (name == "magenta") {
      return "35";
    }
    if (name == "cyan") {
      return "36";
    }
    if (name == "white") {
      return "37";
    }
    if (name == "bright_black") {
      return "90";
    }
    if (name == "bright_red") {
      return "91";
    }
    if (name == "bright_green") {
      return "92";
    }
    if (name == "bright_yellow") {
      return "93";
    }
    if (name == "bright_blue") {
      return "94";
    }
    if (name == "bright_magenta") {
      return "95";
    }
    if (name == "bright_cyan") {
      return "96";
    }
    if (name == "bright_white") {
      return "97";
    }
    return "39";  // default
  }

  static std::string bg_code(const std::string& name) {
    if (name == "black") {
      return "40";
    }
    if (name == "red") {
      return "41";
    }
    if (name == "green") {
      return "42";
    }
    if (name == "yellow") {
      return "43";
    }
    if (name == "blue") {
      return "44";
    }
    if (name == "magenta") {
      return "45";
    }
    if (name == "cyan") {
      return "46";
    }
    if (name == "white") {
      return "47";
    }
    if (name == "bright_black") {
      return "100";
    }
    if (name == "bright_red") {
      return "101";
    }
    if (name == "bright_green") {
      return "102";
    }
    if (name == "bright_yellow") {
      return "103";
    }
    if (name == "bright_blue") {
      return "104";
    }
    if (name == "bright_magenta") {
      return "105";
    }
    if (name == "bright_cyan") {
      return "106";
    }
    if (name == "bright_white") {
      return "107";
    }
    return "";
  }
};

/// All color roles used by the TUI layer.
struct Theme {
  std::string name;  ///< Theme identifier
  Style prompt;      ///< User prompt (nick>)
  Style ai;          ///< AI response text
  Style system;      ///< System messages
  Style error;       ///< Error messages
  Style info;        ///< Info/command output
  Style warning;     ///< Warnings/proposals
  Style banner;      ///< Startup banner
  Style code;        ///< Inline code
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
  return {"mono", {}, {}, {}, {}, {}, {}, {}, {}};
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
