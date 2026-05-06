/**
 * @file kiro_provider.cpp
 * @brief Kiro CLI provider — invokes kiro-cli-chat in headless mode.
 *
 * Uses --no-interactive --wrap=never for clean output.
 * Model list from --list-models -f json includes credit costs.
 */

#include "provider/kiro_provider.h"

#include "exec/exec.h"
#include "json/json.h"
#include "util/util.h"

/// Collapse conversation history into a single prompt.
static std::string collapse_history(const std::vector<Message>& messages) {
  std::string prompt;
  for (const auto& msg : messages) {
    if (msg.role == "system") {
      prompt += "System: " + msg.content + "\n";
    } else if (msg.role == "user") {
      prompt += "User: " + msg.content + "\n";
    } else if (msg.role == "assistant") {
      prompt += "Assistant: " + msg.content + "\n";
    }
  }
  return prompt;
}

/// Send chat to kiro-cli-chat in headless mode and return cleaned response.
std::string KiroProvider::chat(const std::vector<Message>& messages) {
  std::string prompt = collapse_history(messages);
  std::string cmd = "kiro-cli-chat chat --no-interactive --wrap=never " + shell_escape(prompt);
  ExecResult result = cmd_exec(cmd, 60, 100000);
  if (result.exit_code != 0) {
    return "[kiro error: exit " + std::to_string(result.exit_code) + "]";
  }
  // Strip ANSI codes from kiro output (it still emits some color)
  std::string clean = strip_ansi(result.output);
  // Remove the "> " prompt echo at the start
  if (clean.size() > 2 && clean[0] == '>' && clean[1] == ' ') {
    clean = clean.substr(2);
  }
  return clean;
}

/// Streaming not natively supported — sends full response as single token.
std::string KiroProvider::chat_stream(const std::vector<Message>& messages, StreamCallback on_token) {
  std::string response = chat(messages);
  if (on_token) {
    on_token(response);
  }
  return response;
}

/// Query kiro-cli-chat for available models (JSON format).
std::vector<std::string> KiroProvider::list_models() {
  ExecResult result = cmd_exec("kiro-cli-chat chat --list-models -f json", 10, 50000);
  if (result.exit_code != 0) {
    return {"kiro-auto"};
  }
  // Parse JSON model list
  std::vector<std::string> models;
  // Simple extraction: find all "model_name":"..." entries
  size_t pos = 0;
  while ((pos = result.output.find("\"model_name\":\"", pos)) != std::string::npos) {
    pos += 14;
    auto end = result.output.find("\"", pos);
    if (end != std::string::npos) {
      models.push_back(result.output.substr(pos, end - pos));
      pos = end;
    }
  }
  return models.empty() ? std::vector<std::string>{"kiro-auto"} : models;
}

/// Get detailed model info including credit costs from kiro registry.
std::vector<ModelInfo> KiroProvider::get_model_info() {
  ExecResult result = cmd_exec("kiro-cli-chat chat --list-models -f json", 10, 50000);
  std::vector<ModelInfo> infos;
  if (result.exit_code != 0) {
    infos.push_back({"kiro-auto", "?", "none", "amazon-q", 0});
    return infos;
  }
  // Parse each model entry
  size_t pos = 0;
  while ((pos = result.output.find("\"model_name\":\"", pos)) != std::string::npos) {
    pos += 14;
    auto end = result.output.find("\"", pos);
    std::string model_name = result.output.substr(pos, end - pos);
    // Extract rate_multiplier for cost display
    std::string rate = "1.0x";
    auto rate_pos = result.output.find("\"rate_multiplier\":", pos);
    if (rate_pos != std::string::npos && rate_pos < pos + 300) {
      auto rate_start = rate_pos + 18;
      auto rate_end = result.output.find_first_of(",}", rate_start);
      rate = result.output.substr(rate_start, rate_end - rate_start) + "x";
    }
    infos.push_back({model_name, rate, "none", "amazon-q", 0});
    pos = end;
  }
  return infos;
}

bool KiroProvider::is_model_running(const std::string& /*model_name*/) {
  ExecResult result = cmd_exec("which kiro-cli-chat", 3, 200);
  return result.exit_code == 0;
}

std::string KiroProvider::name() const { return "amazon-q"; }

std::string KiroProvider::host() const { return "cloud"; }
