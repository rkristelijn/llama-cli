/**
 * @file util.h
 * @brief Shared utility functions (URL encoding, color mapping).
 */

#ifndef UTIL_H
#define UTIL_H

#include <string>

/// URL-encode a string (spaces → +, special chars → %XX).
std::string url_encode(const std::string& input);

/// Map a color name (e.g. "red", "purple") to its ANSI escape code.
std::string color_name_to_ansi(const std::string& name);

/// Map an ANSI escape code back to a color name.
std::string ansi_to_name(const std::string& code);

#endif  // UTIL_H
