/**
 * @file metrics.cpp
 * @brief Per-model metrics persistence and scoring (ADR-096).
 */

#include "orchestrator/metrics.h"

#include <sys/stat.h>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <sstream>
#include <unordered_map>

#include "json/json.h"
#include "trace/trace.h"

/// Per-model, per-agent stats.
struct ModelStats {
  int calls = 0;                ///< Total number of LLM calls
  int total_tokens = 0;         ///< Sum of prompt + completion tokens
  int total_duration_ms = 0;    ///< Sum of call durations
  int successes = 0;            ///< Calls that returned a valid response
  int64_t last_used_epoch = 0;  ///< Unix timestamp of last use
};

/// Key: "model|agent"
static std::unordered_map<std::string, ModelStats> g_stats;
static std::mutex g_mutex;
static bool g_loaded = false;

/// Build a lookup key from model name and agent role.
static std::string make_key(const std::string& model, const std::string& agent) { return model + "|" + agent; }

/// Determine the metrics file path.
/// Dev mode (Makefile present): .tmp/metrics.json
/// Installed mode: ~/.llama-cli/metrics.json

std::string metrics_path() {
  struct stat st;
  if (stat("Makefile", &st) == 0) {
    mkdir(".tmp", 0750);
    return ".tmp/metrics.json";
  }
  const char* home = std::getenv("HOME");
  std::string dir = std::string(home ? home : ".") + "/.llama-cli";
  mkdir(dir.c_str(), 0750);
  return dir + "/metrics.json";
}

/// Load metrics from disk (once).
static void load_metrics() {
  if (g_loaded) return;
  g_loaded = true;

  std::ifstream f(metrics_path());
  if (!f.is_open()) return;

  std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  // Minimal parse: find each "model|agent" entry
  // Format:
  // {"model|agent":{"calls":N,"tokens":N,"duration_ms":N,"successes":N,"last_used":N},
  // ...}
  size_t pos = 0;
  while ((pos = content.find('"', pos)) != std::string::npos) {
    size_t key_start = pos + 1;
    size_t key_end = content.find('"', key_start);
    if (key_end == std::string::npos) break;
    std::string key = content.substr(key_start, key_end - key_start);
    if (key.find('|') == std::string::npos) {
      pos = key_end + 1;
      continue;
    }

    size_t obj_start = content.find('{', key_end);
    if (obj_start == std::string::npos) break;
    size_t obj_end = content.find('}', obj_start);
    if (obj_end == std::string::npos) break;
    std::string obj = content.substr(obj_start, obj_end - obj_start + 1);

    ModelStats s;
    s.calls = json_extract_int(obj, "calls");
    s.total_tokens = json_extract_int(obj, "tokens");
    s.total_duration_ms = json_extract_int(obj, "duration_ms");
    s.successes = json_extract_int(obj, "successes");
    s.last_used_epoch = json_extract_int(obj, "last_used");
    g_stats[key] = s;
    pos = obj_end + 1;
  }
}

/// Flush metrics to disk.
static void save_metrics() {
  std::ofstream f(metrics_path());
  if (!f.is_open()) return;
  f << "{";
  bool first = true;
  for (const auto& [key, s] : g_stats) {
    if (!first) f << ",";
    first = false;
    f << "\"" << key << "\":{\"calls\":" << s.calls << ",\"tokens\":" << s.total_tokens << ",\"duration_ms\":" << s.total_duration_ms
      << ",\"successes\":" << s.successes << ",\"last_used\":" << s.last_used_epoch << "}";
  }
  f << "}\n";
}

void metrics_record(const std::string& model, const std::string& host, const std::string& agent, int duration_ms, int tokens_prompt,
                    int tokens_completion, bool success) {
  (void)host;
  std::lock_guard<std::mutex> lock(g_mutex);
  load_metrics();

  std::string key = make_key(model, agent);
  auto& s = g_stats[key];
  s.calls++;
  s.total_tokens += tokens_prompt + tokens_completion;
  s.total_duration_ms += duration_ms;
  if (success) s.successes++;
  s.last_used_epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  if (std::getenv("TRACE") && std::getenv("TRACE")[0] != '\0') {
    double tps = (duration_ms > 0) ? ((tokens_prompt + tokens_completion) * 1000.0 / duration_ms) : 0;
    stderr_trace->log(
        "[TRACE] metrics: model=%s agent=%s dur=%dms tok=%d "
        "tps=%.1f success=%d calls=%d\n",
        model.c_str(), agent.c_str(), duration_ms, tokens_prompt + tokens_completion, tps, success ? 1 : 0, s.calls);
  }

  save_metrics();
}

double metrics_score(const std::string& model, const std::string& agent) {
  std::lock_guard<std::mutex> lock(g_mutex);
  load_metrics();

  std::string key = make_key(model, agent);
  auto it = g_stats.find(key);
  if (it == g_stats.end() || it->second.calls == 0) return 0.0;

  const auto& s = it->second;
  double avg_tps = (s.total_duration_ms > 0) ? (s.total_tokens * 1000.0 / s.total_duration_ms) : 0.0;
  double success_rate = static_cast<double>(s.successes) / s.calls;

  // Recency: 1.0 if used in last hour, decays to 0.5 after 24h
  auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  double hours_ago = static_cast<double>(now - s.last_used_epoch) / 3600.0;
  double recency = (hours_ago <= 1.0) ? 1.0 : std::max(0.5, 1.0 - (hours_ago - 1.0) / 46.0);

  double score = avg_tps * success_rate * recency;

  if (std::getenv("TRACE") && std::getenv("TRACE")[0] != '\0') {
    stderr_trace->log(
        "[TRACE] metrics_score: model=%s agent=%s tps=%.1f "
        "success=%.0f%% recency=%.2f → score=%.1f\n",
        model.c_str(), agent.c_str(), avg_tps, success_rate * 100, recency, score);
  }

  return score;
}

// --- Metrics scoring algorithm ---
// score = avg_tokens_per_sec × success_rate × recency_factor
// recency_factor: 1.0 if used in last hour, decays to 0.5 after 24h.
// This prefers warm models (already loaded in GPU memory) over cold ones.
// Cold start: returns 0.0, forcing fallback to complexity-based heuristic.
// After ~10 calls per model, the score becomes reliable for routing.
