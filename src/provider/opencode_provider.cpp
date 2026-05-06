/**
 * @file opencode_provider.cpp
 * @brief OpenCode CLI provider — invokes opencode in non-interactive mode.
 *
 * Collapses conversation history into a single prompt and pipes to opencode.
 * Similar pattern to TgptProvider and KiroProvider.
 */

#include "provider/opencode_provider.h"

#include "exec/exec.h"

/// Collapse conversation into a single prompt string.
static std::string collapse_history(const std::vector<Message>& messages) {
  std::string prompt;
  for (const auto& msg : messages) {
    if (msg.role == "system") {
      prompt += "System: " + msg.content + "\n";
    } else if (msg.role == "user") {
      prompt += msg.content + "\n";
    } else if (msg.role == "assistant") {
      prompt += "Assistant: " + msg.content + "\n";
    }
  }
  return prompt;
}

/// Shell-escape for single quotes.
static std::string shell_escape(const std::string& s) {
  std::string result = "'";
  for (char c : s) {
    if (c == '\'') {
      result += "'\\''";
    } else {
      result += c;
    }
  }
  result += "'";
  return result;
}

/// Strip ANSI escape sequences from output.
static std::string strip_ansi(const std::string& s) {
  std::string result;
  bool in_escape = false;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\033') {
      in_escape = true;
      continue;
    }
    if (in_escape) {
      if ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z')) {
        in_escape = false;
      }
      continue;
    }
    result += s[i];
  }
  return result;
}

std::string OpenCodeProvider::chat(const std::vector<Message>& messages) {
  std::string prompt = collapse_history(messages);
  // Pipe prompt to opencode via echo (non-interactive mode)
  std::string cmd = "echo " + shell_escape(prompt) + " | opencode";
  ExecResult result = cmd_exec(cmd, 120, 100000);
  if (result.exit_code != 0) {
    return "[opencode error: exit " + std::to_string(result.exit_code) + "]";
  }
  return strip_ansi(result.output);
}

std::string OpenCodeProvider::chat_stream(const std::vector<Message>& messages, StreamCallback on_token) {
  // OpenCode doesn't support streaming — return full response as single token
  std::string response = chat(messages);
  if (on_token) {
    on_token(response);
  }
  return response;
}

std::vector<std::string> OpenCodeProvider::list_models() { return {"opencode-default"}; }

std::vector<ModelInfo> OpenCodeProvider::get_model_info() { return {{"opencode-default", "?", "none", "opencode", 0}}; }

bool OpenCodeProvider::is_model_running(const std::string& /*model_name*/) {
  ExecResult result = cmd_exec("which opencode", 3, 200);
  return result.exit_code == 0;
}

std::string OpenCodeProvider::name() const { return "opencode"; }

std::string OpenCodeProvider::host() const { return "cloud"; }
