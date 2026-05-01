/**
 * @file util.cpp
 * @brief Shared utility functions (URL encoding, color mapping).
 */

#include "util/util.h"

#include <cctype>
#include <cstdio>
#include <string>

/// URL-encode a string (spaces → +, special chars → %XX).
std::string url_encode(const std::string& input) {
  std::string out;
  for (char c : input) {
    if (c == ' ') {
      out += '+';
    } else if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += c;
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
      out += buf;
    }
  }
  return out;
}

// NOLINTBEGIN(readability-braces-around-statements)

/// Map a color name to its ANSI escape code.
std::string color_name_to_ansi(const std::string& name) {
  if (name == "black") return "30";
  if (name == "red") return "31";
  if (name == "green") return "32";
  if (name == "yellow") return "33";
  if (name == "blue") return "34";
  if (name == "magenta") return "35";
  if (name == "cyan") return "36";
  if (name == "white") return "37";
  if (name == "bright-red") return "91";
  if (name == "bright-green") return "92";
  if (name == "bright-yellow") return "93";
  if (name == "bright-blue") return "94";
  if (name == "bright-magenta") return "95";
  if (name == "bright-cyan") return "96";
  if (name == "bright-white") return "97";
  if (name == "orange") return "38;5;208";
  if (name == "pink") return "38;5;213";
  if (name == "purple") return "38;5;129";
  if (name == "lime") return "38;5;118";
  if (name == "none" || name == "default") return "";
  return "";
}

/// Map an ANSI escape code back to a color name.
std::string ansi_to_name(const std::string& code) {
  if (code == "30") return "black";
  if (code == "31") return "red";
  if (code == "32") return "green";
  if (code == "33") return "yellow";
  if (code == "34") return "blue";
  if (code == "35") return "magenta";
  if (code == "36") return "cyan";
  if (code == "37") return "white";
  if (code == "91") return "bright-red";
  if (code == "92") return "bright-green";
  if (code == "93") return "bright-yellow";
  if (code == "94") return "bright-blue";
  if (code == "95") return "bright-magenta";
  if (code == "96") return "bright-cyan";
  if (code == "97") return "bright-white";
  if (code == "38;5;208") return "orange";
  if (code == "38;5;213") return "pink";
  if (code == "38;5;129") return "purple";
  if (code == "38;5;118") return "lime";
  return "none";
}

// NOLINTEND(readability-braces-around-statements)
