/**
 * @file session.h
 * @brief Session persistence — load/save conversation history as JSON (ADR-056).
 */

#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <vector>

#include "ollama/ollama.h"  // Message struct

/// Load conversation history from a session JSON file.
/// Returns empty vector if file doesn't exist yet.
std::vector<Message> load_session(const std::string& path);

/// Save conversation history to a session JSON file.
void save_session(const std::string& path, const std::vector<Message>& msgs);

#endif  // SESSION_H
