/**
 * @file tgpt_provider.cpp
 * @brief tgpt provider — invokes tgpt CLI as subprocess.
 *
 * Uses -q (quiet, no spinner) and -p (prompt) flags.
 * History is collapsed into a single prompt since tgpt is stateless.
 */

#include "provider/tgpt_provider.h"

#include "exec/exec.h"
#include "util/util.h"

/// Collapse conversation history into a single prompt string.
/// Format: "System: ...\nUser: ...\nAssistant: ...\nUser: ..."
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

std::string TgptProvider::chat(const std::vector<Message>& messages) {
  std::string prompt = collapse_history(messages);
  // -q suppresses the loading spinner, -w forces raw output (no markdown)
  std::string cmd = "tgpt -q -w " + shell_escape(prompt);
  ExecResult result = cmd_exec(cmd, 30, 100000);
  if (result.exit_code != 0) {
    return "[tgpt error: exit " + std::to_string(result.exit_code) + "]";
  }
  // Detect error responses (API failures, rate limits)
  if (result.output.find("Error:") != std::string::npos || result.output.find("\"error\"") != std::string::npos) {
    return "[tgpt error: provider returned an error]";
  }
  return result.output;
}

std::string TgptProvider::chat_stream(const std::vector<Message>& messages, StreamCallback on_token) {
  // tgpt doesn't support real streaming via CLI — get full response then emit
  std::string response = chat(messages);
  if (on_token) {
    on_token(response);
  }
  return response;
}

std::vector<std::string> TgptProvider::list_models() { return {"tgpt-default"}; }

std::vector<ModelInfo> TgptProvider::get_model_info() { return {{"tgpt-default", "unknown", "none", "tgpt", 0}}; }

bool TgptProvider::is_model_running(const std::string& /*model_name*/) {
  // Check if tgpt binary exists
  ExecResult result = cmd_exec("which tgpt", 5, 1000);
  return result.exit_code == 0;
}

std::string TgptProvider::name() const { return "tgpt"; }

std::string TgptProvider::host() const { return "local-cli"; }
