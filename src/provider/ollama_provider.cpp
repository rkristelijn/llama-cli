/**
 * @file ollama_provider.cpp
 * @brief Ollama provider implementation — thin wrapper around existing API functions.
 */

#include "provider/ollama_provider.h"

#include "ollama/ollama.h"

OllamaProvider::OllamaProvider(const Config& cfg) : cfg_(cfg) {}

std::string OllamaProvider::chat(const std::vector<Message>& messages) { return ollama_chat(cfg_, messages); }

std::string OllamaProvider::chat_stream(const std::vector<Message>& messages, StreamCallback on_token) {
  return ollama_chat_stream(cfg_, messages, on_token);
}

std::vector<std::string> OllamaProvider::list_models() { return get_available_models(cfg_); }

std::vector<ModelInfo> OllamaProvider::get_model_info() { return ::get_model_info(cfg_); }

bool OllamaProvider::is_model_running(const std::string& model_name) { return ::is_model_running(cfg_, model_name); }

std::string OllamaProvider::name() const { return "ollama"; }

std::string OllamaProvider::host() const { return cfg_.host + ":" + cfg_.port; }
