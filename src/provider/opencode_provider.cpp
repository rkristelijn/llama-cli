/**
 * @file opencode_provider.cpp
 * @brief OpenCode CLI provider — invokes opencode in non-interactive mode.
 *
 * Collapses conversation history into a single prompt and pipes to opencode.
 * Similar pattern to TgptProvider and KiroProvider.
 */

#include "provider/opencode_provider.h"

#include "exec/exec.h"
#include "util/util.h"

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

std::string OpenCodeProvider::chat(const std::vector<Message>& messages) {
  std::string prompt = collapse_history(messages);
  // Pipe prompt to opencode via echo (non-interactive mode).
  // OpenCode reads from stdin when not attached to a TTY.
  std::string cmd = "echo " + shell_escape(prompt) + " | opencode";
  ExecResult result = cmd_exec(cmd, 120, 100000);
  if (result.exit_code != 0) {
    return "[opencode error: exit " + std::to_string(result.exit_code) + "]";
  }
  // Strip ANSI — opencode emits color even in pipe mode
  return strip_ansi(result.output);
}

/// Streaming not supported by opencode CLI — emit full response as one token.
std::string OpenCodeProvider::chat_stream(const std::vector<Message>& messages, StreamCallback on_token) {
  std::string response = chat(messages);
  if (on_token) {
    on_token(response);
  }
  return response;
}

/// Single fixed model — opencode manages its own model selection internally.
std::vector<std::string> OpenCodeProvider::list_models() { return {"opencode-default"}; }

/// No detailed model info available from opencode CLI.
std::vector<ModelInfo> OpenCodeProvider::get_model_info() { return {{"opencode-default", "?", "none", "opencode", 0}}; }

/// Check if opencode binary is on PATH.
bool OpenCodeProvider::is_model_running(const std::string& /*model_name*/) {
  ExecResult result = cmd_exec("which opencode", 3, 200);
  return result.exit_code == 0;
}

/// Provider display name for registry and /provider command.
std::string OpenCodeProvider::name() const { return "opencode"; }

/// Host identifier — always "cloud" since opencode manages its own backend.
std::string OpenCodeProvider::host() const { return "cloud"; }
