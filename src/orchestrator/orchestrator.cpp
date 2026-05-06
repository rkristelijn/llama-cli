/**
 * @file orchestrator.cpp
 * @brief Multi-agent orchestrator (ADR-096).
 *
 * Routes complex prompts to the best available agent tool:
 * 1. Amazon Q (if `q` binary available) — best for agentic coding
 * 2. OpenCode (if binary available) — multi-agent coding
 * 3. Kiro CLI (if binary available) — Claude via credits
 * 4. Ollama plan-loop fallback — local decomposition + execution
 *
 * Simple prompts always go through the existing single-LLM path.
 */

#include "orchestrator/orchestrator.h"

#include <fstream>
#include <sstream>

#include "config/config.h"
#include "exec/exec.h"
#include "orchestrator/subagent.h"
#include "provider/provider_factory.h"
#include "trace/trace.h"

/// Check if a CLI tool is available on PATH.
static bool tool_available(const std::string& name) {
  ExecResult r = cmd_exec("which " + name, 3, 200);
  return r.exit_code == 0;
}

/// Load the plan prompt template from res/agents/prompts/plan.txt.
static std::string load_plan_prompt() {
  std::ifstream f("res/agents/prompts/plan.txt");
  if (!f.is_open()) return "";
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

/// Ask the local Ollama model to decompose a complex prompt into a plan.
/// Returns a TaskPlan (may be empty if LLM output is unparseable).
static TaskPlan local_plan(const std::string& prompt) {
  std::string system_prompt = load_plan_prompt();
  if (system_prompt.empty()) {
    // Inline fallback if template file missing
    system_prompt =
        "Output a JSON task plan: "
        "{\"goal\":\"...\",\"steps\":[{\"id\":1,\"tool\":\"read|write|str_replace|exec\",\"action\":\"...\",\"input\":{}}]}";
  }
  std::vector<Message> msgs = {{"system", system_prompt}, {"user", prompt}};
  Config cfg = Config::instance();
  auto provider = create_provider(cfg);
  std::string response = provider->chat(msgs);

  // Try to parse JSON from the response
  TaskPlan plan = parse_task_plan(response);
  if (plan.steps.empty()) {
    plan.goal = prompt;
  }
  return plan;
}

/// Format a plan for display to the user.
static std::string format_plan(const TaskPlan& plan) {
  std::ostringstream out;
  out << "[plan] " << plan.goal << "\n";
  for (const auto& step : plan.steps) {
    out << "  " << step.id << ". [" << tool_to_string(step.tool) << "] " << step.action << "\n";
  }
  return out.str();
}

bool should_orchestrate(const std::string& prompt) {
  Complexity level = classify_prompt(prompt);
  return level == Complexity::Complex;
}

OrchestratorResult orchestrate(const std::string& prompt, const std::vector<Message>& history) {
  OrchestratorResult result;
  result.plan.goal = prompt;

  Complexity level = classify_prompt(prompt);
  if (level != Complexity::Complex) {
    result.used_plan = false;
    return result;
  }

  if (std::getenv("TRACE") && std::getenv("TRACE")[0] != '\0') {
    stderr_trace->log("[TRACE] orchestrate: complex prompt, checking available agents\n");
  }

  // Try external agents in priority order (best agentic capability first)
  if (tool_available("q")) {
    result.response = invoke_subagent("q", prompt, history);
    result.used_plan = true;
    return result;
  }
  if (tool_available("opencode")) {
    result.response = invoke_subagent("opencode", prompt, history);
    result.used_plan = true;
    return result;
  }
  if (tool_available("kiro-cli-chat")) {
    result.response = invoke_subagent("kiro", prompt, history);
    result.used_plan = true;
    return result;
  }

  // Fallback: local Ollama plan-loop (ADR-096 Phase C)
  // Ask the model to decompose, show plan to user, then pass through
  // for normal execution (annotations will handle the actual work).
  TaskPlan plan = local_plan(prompt);
  if (!plan.steps.empty()) {
    result.plan = plan;
    result.response = format_plan(plan);
    result.used_plan = true;
    return result;
  }

  // Plan parsing failed — fall through to single-LLM path
  result.used_plan = false;
  return result;
}
