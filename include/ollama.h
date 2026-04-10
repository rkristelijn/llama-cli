// ollama.h — Ollama API client
// Handles HTTP communication with a local Ollama instance.

#ifndef OLLAMA_H
#define OLLAMA_H

#include "config.h"
#include <string>

// Send a prompt to Ollama and return the response text.
// Returns empty string on connection failure.
std::string ollama_generate(const Config &cfg, const std::string &prompt);

#endif
