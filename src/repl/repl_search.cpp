/**
 * @file repl_search.cpp
 * @brief Web search integration for the REPL (ADR-057).
 *
 * SRP: Docker/SearXNG lifecycle and web search queries.
 * Extracted from repl.cpp to reduce file size (ADR-065).
 *
 * @see docs/adr/adr-057-web-search-integration.md
 */

#include "repl/repl_search.h"

#include <string>

#include "exec/exec.h"
#include "json/json.h"
#include "trace/trace.h"
#include "util/util.h"

// --- SearXNG lifecycle ---
// Ensures Docker is running, then ensures the searxng container is up.
// Probes the search URL first — if already responding, returns immediately.

void ensure_searxng(const Config& cfg, std::ostream& out) {
  bool trace = cfg.trace;
  // Quick check: is SearXNG already responding?
  if (trace) {
    stderr_trace->log("[TRACE] web-search: probing %s\n", cfg.search_url.c_str());
  }
  auto probe = cmd_exec("curl -sf -o /dev/null -m 2 '" + cfg.search_url + "'", 3, 100);
  if (probe.exit_code == 0) {
    if (trace) {
      stderr_trace->log("[TRACE] web-search: SearXNG already running\n");
    }
    return;
  }

  // Check if docker CLI is available
  if (trace) {
    stderr_trace->log("[TRACE] web-search: checking docker CLI\n");
  }
  auto docker_check = cmd_exec("docker --version", 3, 200);
  if (docker_check.exit_code != 0) {
    out << "\033[33m[web-search] docker not found — install Docker to enable web search\033[0m\n";
    return;
  }

  // Check if Docker daemon is running
  if (trace) {
    stderr_trace->log("[TRACE] web-search: checking Docker daemon\n");
  }
  auto daemon_check = cmd_exec("docker info", 5, 500);
  if (daemon_check.exit_code != 0) {
    out << "\033[33m[web-search] starting Docker...\033[0m\n";
    if (trace) {
      stderr_trace->log("[TRACE] web-search: starting Docker daemon\n");
    }
#ifdef __APPLE__
    cmd_exec("open -a Docker", 5, 100);
#else
    cmd_exec("sudo systemctl start docker", 10, 200);
#endif
    // Wait for daemon to be ready (up to 30s)
    for (int i = 0; i < 15; ++i) {
      auto ready = cmd_exec("docker info", 3, 500);
      if (ready.exit_code == 0) {
        if (trace) {
          stderr_trace->log("[TRACE] web-search: Docker daemon ready\n");
        }
        break;
      }
      cmd_exec("sleep 2", 3, 10);
    }
  }

  // Check if searxng container exists and is running
  if (trace) {
    stderr_trace->log("[TRACE] web-search: checking searxng container\n");
  }
  auto running = cmd_exec("docker ps -q -f name=searxng", 5, 200);
  if (!running.output.empty()) {
    if (trace) {
      stderr_trace->log("[TRACE] web-search: container already running\n");
    }
    return;
  }

  // Check if container exists but is stopped
  auto exists = cmd_exec("docker ps -aq -f name=searxng", 5, 200);
  if (!exists.output.empty()) {
    out << "\033[33m[web-search] starting SearXNG container...\033[0m\n";
    if (trace) {
      stderr_trace->log("[TRACE] web-search: docker start searxng\n");
    }
    cmd_exec("docker start searxng", 10, 200);
  } else {
    out << "\033[33m[web-search] pulling and starting SearXNG...\033[0m\n";
    if (trace) {
      stderr_trace->log("[TRACE] web-search: docker compose up searxng\n");
    }
    cmd_exec("docker compose -f .config/docker-compose.yml up -d", 60, 5000);
  }

  // Wait for SearXNG to respond (up to 15s)
  for (int i = 0; i < 15; ++i) {
    auto ready = cmd_exec("curl -sf -o /dev/null -m 1 '" + cfg.search_url + "'", 2, 100);
    if (ready.exit_code == 0) {
      out << "\033[32m[web-search] SearXNG ready\033[0m\n";
      if (trace) {
        stderr_trace->log("[TRACE] web-search: SearXNG responding\n");
      }
      return;
    }
    cmd_exec("sleep 1", 2, 10);
  }
  out << "\033[33m[web-search] SearXNG not responding — search may fail\033[0m\n";
}

// --- Web search query ---
// Calls SearXNG JSON API, parses up to 5 results from the response.
// Returns formatted markdown-style list of results with titles and URLs.
// Falls back gracefully: returns error message on network failure or empty results.

std::string web_search(const std::string& query, const Config& cfg) {
  bool trace = cfg.trace;
  // Build SearXNG JSON API URL
  std::string url = cfg.search_url + "/search?q=" + url_encode(query) + "&format=json&language=" + url_encode(cfg.search_lang);
  if (!cfg.search_location.empty()) {
    url += "&location=" + url_encode(cfg.search_location);
  }

  if (trace) {
    stderr_trace->log("[TRACE] web-search: GET %s\n", url.c_str());
  }
  std::string cmd = "curl -sfL '" + url + "'";
  auto result = cmd_exec(cmd, 10, 200000);
  if (result.exit_code != 0 || result.output.empty()) {
    if (trace) {
      stderr_trace->log("[TRACE] web-search: curl failed exit=%d output=%s\n", result.exit_code, result.output.c_str());
    }
    return "[web search failed]";
  }
  if (trace) {
    stderr_trace->log("[TRACE] web-search: response %zu bytes\n", result.output.size());
  }

  // Parse results array — walk through JSON objects in "results":[...]
  // SearXNG returns: {"results": [{"title": "...", "content": "...", "url": "..."}, ...]}
  const std::string& body = result.output;
  auto arr_key = body.find("\"results\"");
  if (arr_key == std::string::npos) {
    return "[web search: no results for '" + query + "']";
  }
  // Find opening bracket of the results array
  auto arr_start = body.find('[', arr_key);
  if (arr_start == std::string::npos) {
    return "[web search: no results for '" + query + "']";
  }

  std::string out;
  int count = 0;
  size_t pos = arr_start + 1;
  // Extract up to 5 result objects from the array
  while (count < 5 && pos < body.size()) {
    auto obj_start = body.find('{', pos);
    if (obj_start == std::string::npos) {
      break;
    }
    std::string obj = json_extract_object_at(body, obj_start);
    if (obj.empty()) {
      break;
    }
    pos = obj_start + obj.size();

    std::string title = json_extract_string(obj, "title");
    std::string content = json_extract_string(obj, "content");
    std::string link = json_extract_string(obj, "url");
    if (!content.empty()) {
      out += "- " + title + ": " + content;
      if (!link.empty()) {
        out += " (" + link + ")";
      }
      out += "\n";
      ++count;
    }
  }
  if (out.empty()) {
    return "[web search: no results for '" + query + "']";
  }
  if (trace) {
    stderr_trace->log("[TRACE] web-search: %d results for '%s'\n", count, query.c_str());
  }
  return "[web search results for '" + query + "']\n" + out;
}
