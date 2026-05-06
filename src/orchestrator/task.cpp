/**
 * @file task.cpp
 * @brief Task schema parsing and serialization (ADR-096).
 */

#include "orchestrator/task.h"

#include <cstdlib>
#include <sstream>

#include "json/json.h"
#include "trace/trace.h"

std::string tool_to_string(Tool t) {
  switch (t) {
    case Tool::Read:
      return "read";
    case Tool::Write:
      return "write";
    case Tool::StrReplace:
      return "str_replace";
    case Tool::Exec:
      return "exec";
    case Tool::Git:
      return "git";
  }
  return "read";
}

Tool string_to_tool(const std::string& s) {
  if (s == "write") return Tool::Write;
  if (s == "str_replace") return Tool::StrReplace;
  if (s == "exec") return Tool::Exec;
  if (s == "git") return Tool::Git;
  return Tool::Read;
}

std::string status_to_string(Status s) {
  switch (s) {
    case Status::Pending:
      return "pending";
    case Status::Running:
      return "running";
    case Status::Completed:
      return "completed";
    case Status::Failed:
      return "failed";
    case Status::Skipped:
      return "skipped";
  }
  return "pending";
}

/// Parse a JSON task plan from LLM output into a structured TaskPlan.
/// Handles malformed JSON gracefully — returns empty plan on failure.
/// Extracts goal, then iterates over the "steps" array parsing each step
/// object.
TaskPlan parse_task_plan(const std::string& json) {
  TaskPlan plan;
  plan.goal = json_extract_string(json, "goal");

  if (std::getenv("TRACE") && std::getenv("TRACE")[0] != '\0') {
    stderr_trace->log("[TRACE] parse_task_plan: goal=%s json=%zu chars\n", plan.goal.c_str(), json.size());
  }

  // Find "steps" array
  auto steps_start = json.find("\"steps\"");
  if (steps_start == std::string::npos) return plan;

  auto arr_start = json.find('[', steps_start);
  if (arr_start == std::string::npos) return plan;

  // Parse each step object in the array
  size_t pos = arr_start + 1;
  while (pos < json.size()) {
    auto obj_start = json.find('{', pos);
    if (obj_start == std::string::npos) break;

    // Find matching closing brace
    int depth = 0;
    size_t obj_end = obj_start;
    for (size_t i = obj_start; i < json.size(); i++) {
      if (json[i] == '{') depth++;
      if (json[i] == '}') depth--;
      if (depth == 0) {
        obj_end = i;
        break;
      }
    }

    std::string obj = json.substr(obj_start, obj_end - obj_start + 1);

    TaskStep step;
    step.id = json_extract_int(obj, "id");
    step.tool = string_to_tool(json_extract_string(obj, "tool"));
    step.action = json_extract_string(obj, "action");

    // Parse input object
    std::string input_str = json_extract_object(obj, "input");
    if (!input_str.empty()) {
      // Extract known keys from input
      for (const auto& key : {"path", "cmd", "old", "new", "content", "branch"}) {
        std::string val = json_extract_string(input_str, key);
        if (!val.empty()) {
          step.input.emplace_back(key, val);
        }
      }
    }

    plan.steps.push_back(step);
    pos = obj_end + 1;

    // Stop at end of array
    auto next = json.find_first_not_of(" \t\n\r,", pos);
    if (next != std::string::npos && json[next] == ']') break;
  }

  if (std::getenv("TRACE") && std::getenv("TRACE")[0] != '\0') {
    stderr_trace->log("[TRACE] parse_task_plan: parsed %zu steps\n", plan.steps.size());
  }

  return plan;
}

/// Serialize a TaskPlan to a JSON string for logging, debugging, or
/// persistence. Output is compact single-line JSON (no pretty-printing).
std::string serialize_task_plan(const TaskPlan& plan) {
  std::ostringstream ss;
  ss << "{\"goal\":\"" << plan.goal << "\",\"status\":\"" << status_to_string(plan.status) << "\",\"steps\":[";
  for (size_t i = 0; i < plan.steps.size(); i++) {
    const auto& s = plan.steps[i];
    if (i > 0) ss << ",";
    ss << "{\"id\":" << s.id << ",\"tool\":\"" << tool_to_string(s.tool) << "\",\"action\":\"" << s.action << "\",\"status\":\""
       << status_to_string(s.status) << "\"";
    if (!s.input.empty()) {
      ss << ",\"input\":{";
      for (size_t j = 0; j < s.input.size(); j++) {
        if (j > 0) ss << ",";
        ss << "\"" << s.input[j].first << "\":\"" << s.input[j].second << "\"";
      }
      ss << "}";
    }
    ss << "}";
  }
  ss << "]}";
  return ss.str();
}
