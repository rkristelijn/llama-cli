/**
 * @file subagent.cpp
 * @brief Subagent invocation via at-mention syntax (ADR-096 Phase 4).
 *
 * Routes at-q to Amazon Q CLI, at-opencode to OpenCode CLI,
 * and at-explore/at-plan/at-general to internal Ollama agents with role prompts.
 */

#include "orchestrator/subagent.h"

#include <fstream>
#include <sstream>

#include "exec/exec.h"
#include "logging/logger.h"
#include "orchestrator/agent_config.h"
#include "orchestrator/prompt_template.h"
#include "provider/provider.h"
#include "provider/provider_factory.h"
#include "trace/trace.h"
#include "util/util.h"

MentionResult parse_mention(const std::string& input) {
  MentionResult r;
  // Must start with @ to be a mention
  if (input.empty() || input[0] != '@') return r;

  // Find end of agent name (first space or end of string)
  size_t space = input.find(' ', 1);
  if (space == std::string::npos) {
    // Just "@agent" with no prompt — not actionable
    return r;
  }
  r.agent = input.substr(1, space - 1);
  r.prompt = input.substr(space + 1);
  r.has_mention = !r.agent.empty() && !r.prompt.empty();
  return r;
}

/// Invoke Amazon Q Developer CLI (`q chat`).
static std::string invoke_q(const std::string& prompt) {
  std::string cmd = "q chat " + shell_escape(prompt);
  ExecResult result = cmd_exec(cmd, 120, 100000);
  if (result.exit_code != 0) {
    return "[q error: exit " + std::to_string(result.exit_code) + "]";
  }
  return strip_ansi(result.output);
}

/// Invoke OpenCode CLI (`opencode`).
static std::string invoke_opencode(const std::string& prompt) {
  // OpenCode uses stdin for non-interactive mode
  std::string cmd = "echo " + shell_escape(prompt) + " | opencode";
  ExecResult result = cmd_exec(cmd, 120, 100000);
  if (result.exit_code != 0) {
    return "[opencode error: exit " + std::to_string(result.exit_code) + "]";
  }
  return strip_ansi(result.output);
}

/// Invoke an internal agent via the current Ollama provider with role prompt.
/// Builds a focused context: system prompt + last 4 messages + user prompt.
/// This keeps the subagent's context small and focused on the task.
static std::string invoke_internal(const std::string& agent, const std::string& prompt, const std::vector<Message>& history) {
  // Load the agent's prompt template for its role-specific system prompt
  static std::vector<AgentConfig> agents = load_agent_configs();
  const AgentConfig* cfg = find_agent_config(agents, agent);
  std::string system_prompt;
  if (cfg && !cfg->prompt_file.empty()) {
    std::string path = "res/agents/prompts/" + cfg->prompt_file;
    std::ifstream f(path);
    if (f.is_open()) {
      std::ostringstream ss;
      ss << f.rdbuf();
      system_prompt = ss.str();
    }
  }

  // Build a minimal message history: system prompt + user prompt
  std::vector<Message> msgs;
  if (!system_prompt.empty()) {
    msgs.push_back({"system", system_prompt});
  }
  // Include last few messages for context (max 4)
  size_t start = history.size() > 4 ? history.size() - 4 : 0;
  for (size_t i = start; i < history.size(); i++) {
    if (history[i].role != "system") {
      msgs.push_back(history[i]);
    }
  }
  msgs.push_back({"user", prompt});

  // Use the default provider (Ollama) for internal agents
  Config app_cfg = Config::instance();
  auto provider = create_provider(app_cfg);
  return provider->chat(msgs);
}

/// Route to the named subagent. External agents (q, opencode) use subprocess,
/// internal agents (explore, plan, general, reviewer) use Ollama with role prompts.
std::string invoke_subagent(const std::string& agent, const std::string& prompt, const std::vector<Message>& history) {
  LOG_FEATURE("subagent_invoke");
  LOG_EVENT("orchestrator", "subagent_invoke", agent, prompt, 0, 0, 0);

  // External CLI agents — best agentic capability
  if (agent == "q" || agent == "amazon-q") {
    return invoke_q(prompt);
  }
  if (agent == "opencode" || agent == "oc") {
    return invoke_opencode(prompt);
  }
  // Internal agents: use Ollama with role-specific system prompt
  return invoke_internal(agent, prompt, history);
}
