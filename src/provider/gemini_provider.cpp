/**
 * @file gemini_provider.cpp
 * @brief Gemini provider — invokes gemini CLI in headless mode.
 *
 * Uses -p (prompt) flag for non-interactive mode.
 * History is collapsed into a single prompt since the CLI is stateless.
 */

#include "provider/gemini_provider.h"

#include "exec/exec.h"

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

/// Shell-escape a string for safe embedding in single quotes.
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

std::string GeminiProvider::chat(const std::vector<Message>& messages) {
  std::string prompt = collapse_history(messages);
  // -p runs in headless (non-interactive) mode
  std::string cmd = "gemini -p " + shell_escape(prompt);
  ExecResult result = cmd_exec(cmd, 120, 100000);
  if (result.exit_code != 0) {
    return "[gemini error: exit " + std::to_string(result.exit_code) + "]";
  }
  // Detect error dumps: stacktraces, JSON errors, rate limits
  if (result.output.find("Error:") != std::string::npos || result.output.find("\"error\"") != std::string::npos ||
      result.output.find("at async") != std::string::npos || result.output.find("RESOURCE_EXHAUSTED") != std::string::npos) {
    // Extract just the error message, not the full stacktrace
    auto msg_pos = result.output.find("\"message\":");
    if (msg_pos != std::string::npos) {
      auto start = result.output.find("\"", msg_pos + 10) + 1;
      auto end = result.output.find("\"", start);
      return "[gemini error: " + result.output.substr(start, end - start) + "]";
    }
    return "[gemini error: provider returned an error instead of a response]";
  }
  return result.output;
}

std::string GeminiProvider::chat_stream(const std::vector<Message>& messages, StreamCallback on_token) {
  // Gemini CLI doesn't support streaming via subprocess — emit full response
  std::string response = chat(messages);
  if (on_token) {
    on_token(response);
  }
  return response;
}

std::vector<std::string> GeminiProvider::list_models() { return {"gemini-cli"}; }

std::vector<ModelInfo> GeminiProvider::get_model_info() { return {{"gemini-cli", "unknown", "none", "gemini", 0}}; }

bool GeminiProvider::is_model_running(const std::string& /*model_name*/) {
  // Check if gemini binary exists
  ExecResult result = cmd_exec("which gemini", 5, 1000);
  return result.exit_code == 0;
}

std::string GeminiProvider::name() const { return "gemini"; }

std::string GeminiProvider::host() const { return "local-cli"; }
