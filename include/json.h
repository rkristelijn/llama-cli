// json.h — Minimal JSON helpers
// Extracts values from JSON strings without external dependencies.

#ifndef JSON_H
#define JSON_H

#include <string>

// Extract a string value for a given key from a JSON object.
// Returns empty string if key is not found.
std::string json_extract_string(const std::string& json, const std::string& key);

#endif
