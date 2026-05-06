/**
 * @file planner.cpp
 * @brief Prompt complexity classifier and route picker.
 *
 * Classification is pure string heuristics — no LLM call overhead.
 * Routing maps complexity to known hosts/models from config.
 */

#include "planner/planner.h"

#include <algorithm>
#include <sstream>

#include "config/config.h"
#include "ollama/ollama.h"

/// Count words in a string (split by whitespace).
static int word_count(const std::string& s) {
  std::istringstream iss(s);
  int count = 0;
  std::string word;
  while (iss >> word) {
    count++;
  }
  return count;
}

/// Check if prompt contains code-related keywords.
static bool has_code_markers(const std::string& prompt) {
  // Lowercase copy for matching
  std::string lower = prompt;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });

  // Code keywords that suggest programming tasks
  const char* markers[] = {"function", "class ", "def ", "import ", "include", "struct ",  "async",     "await",
                           "->",       "=>",     "```",  "compile", "debug",   "refactor", "implement", "algorithm"};
  for (const auto& m : markers) {
    if (lower.find(m) != std::string::npos) {
      return true;
    }
  }
  return false;
}

/// Check for complexity indicators (multi-part questions, analysis requests).
static bool has_complexity_markers(const std::string& prompt) {
  std::string lower = prompt;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });

  const char* markers[] = {"explain",      "compare", "design", "architect", "tradeoff",     "pros and cons",
                           "step by step", "analyze", "review", "why does",  "how would you"};
  for (const auto& m : markers) {
    if (lower.find(m) != std::string::npos) {
      return true;
    }
  }
  return false;
}

Complexity classify_prompt(const std::string& prompt) {
  int words = word_count(prompt);
  int questions = std::count(prompt.begin(), prompt.end(), '?');
  bool code = has_code_markers(prompt);
  bool complex_markers = has_complexity_markers(prompt);

  // Complex: code tasks, multi-question, long prompts, or analysis requests
  if (code || questions > 2 || words > 90 || complex_markers) {
    return Complexity::Complex;
  }
  // Medium: moderate length or has a question
  if (words > 30 || questions > 0) {
    return Complexity::Medium;
  }
  // Simple: short factual or greeting
  return Complexity::Simple;
}

/// Find a model matching a size preference on a given host.
/// Probes the host's /api/tags to find models by parameter size.
static RouteTarget find_model_on_host(const std::string& host_port, double min_params, double max_params) {
  Config tmp;
  auto colon = host_port.find(':');
  tmp.host = (colon != std::string::npos) ? host_port.substr(0, colon) : host_port;
  tmp.port = (colon != std::string::npos) ? host_port.substr(colon + 1) : "11434";
  tmp.timeout = 3;

  auto infos = get_model_info(tmp);
  for (const auto& info : infos) {
    // Skip embedding models
    if (info.family.find("embed") != std::string::npos) {
      continue;
    }
    double params = 0;
    try {
      params = std::stod(info.params);
    } catch (...) {
      continue;
    }
    if (params >= min_params && params <= max_params) {
      return {host_port, info.name, ""};
    }
  }
  return {"", "", ""};
}

RouteTarget pick_route(Complexity level, const std::vector<std::string>& hosts) {
  // Strategy: scan hosts for models in the right size range
  switch (level) {
    case Complexity::Simple: {
      // Prefer small models (1-4B) on any host — fast response
      for (const auto& h : hosts) {
        auto r = find_model_on_host(h, 1.0, 4.0);
        if (!r.host.empty()) {
          r.reason = "simple → small model (fast)";
          return r;
        }
      }
      break;
    }
    case Complexity::Medium: {
      // Prefer mid-size models (7-16B)
      for (const auto& h : hosts) {
        auto r = find_model_on_host(h, 7.0, 16.0);
        if (!r.host.empty()) {
          r.reason = "medium → mid model";
          return r;
        }
      }
      break;
    }
    case Complexity::Complex: {
      // Prefer large models (20B+)
      for (const auto& h : hosts) {
        auto r = find_model_on_host(h, 20.0, 999.0);
        if (!r.host.empty()) {
          r.reason = "complex → large model";
          return r;
        }
      }
      break;
    }
  }
  // Fallback: use current config
  const auto& cfg = Config::instance();
  return {cfg.host + ":" + cfg.port, cfg.model, "fallback → current config"};
}

RouteTarget plan_route(const std::string& prompt, const std::vector<std::string>& hosts) {
  Complexity level = classify_prompt(prompt);
  return pick_route(level, hosts);
}
