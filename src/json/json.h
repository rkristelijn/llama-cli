/**
 * @file json.h
 * @brief Minimal JSON string extraction for Ollama API responses.
 */

#ifndef JSON_H
#define JSON_H

#include <string>

/// Extract a JSON string value by key: "key":"value"
std::string json_extract_string(const std::string& json, const std::string& key);

/// Extract a JSON object value by key: "key":{...}
std::string json_extract_object(const std::string& json, const std::string& key);

/// Extract a JSON integer value by key: "key":123
int json_extract_int(const std::string& json, const std::string& key);

#endif  // JSON_H
