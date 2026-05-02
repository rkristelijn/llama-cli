/**
 * @file repl_chat.cpp
 * @brief Chat loop logic: send prompts, handle responses, manage follow-ups.
 *
 * SRP: LLM interaction (spinner, streaming, annotation dispatch, follow-ups).
 * Extracted from repl.cpp to reduce file size (ADR-065).
 *
 * @see docs/adr/adr-012-interactive-repl.md
 * @see docs/adr/adr-028-execution-limits.md
 */

#include "repl/repl_chat.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "annotation/annotation.h"
#include "logging/logger.h"
#include "repl/repl_annotations.h"
#include "repl/repl_search.h"
#include "sync/sync.h"
#include "trace/trace.h"
#include "tui/tui.h"

/// Global interrupt flag (defined in repl.cpp)
extern volatile sig_atomic_t g_interrupted;

/// SIGINT handler (defined in repl.cpp)
void sigint_handler(int);

// --- Colorize helper ---
// Re-applies AI color after any ANSI reset inside markdown rendering.

static std::string colorize_ai(const std::string& text, const ReplState& s) {
  if (!s.color || s.ai_color.empty()) {
    return text;
  }
  std::string color_code = "\033[" + s.ai_color + "m";
  std::string result = color_code;
  std::string reset = "\033[0m";
  size_t pos = 0;
  size_t found;
  while ((found = text.find(reset, pos)) != std::string::npos) {
    result += text.substr(pos, found - pos + reset.size()) + color_code;
    pos = found + reset.size();
  }
  result += text.substr(pos) + "\033[0m";
  return result;
}

// --- Chat execution ---
// interruptible_chat: runs LLM on a thread, polls g_interrupted every 50ms.
// Thread safety: history is copied into a shared_ptr so detach is safe.

/// Run chat on a thread, interruptible by Ctrl+C.
/// Polls g_interrupted every 50ms. If interrupted, detaches the HTTP thread.
/// Thread safety: history is copied into a shared_ptr so detach is safe —
/// no dangling reference to s.history after Ctrl+C.
// pmccabe:skip-complexity
static std::string interruptible_chat(ReplState& s) {
  auto result = std::make_shared<std::string>();
  auto done = std::make_shared<std::atomic<bool>>(false);

  if (s.stream_chat) {
    // Streaming: spinner runs until first token, then live output.
    // The spinner is stopped on first token arrival (first_token flag).
    // After that, tokens are rendered directly via StreamRenderer.
    auto first_token = std::make_shared<std::atomic<bool>>(false);
    auto renderer = std::make_shared<StreamRenderer>(s.out, s.color && s.markdown);
    Spinner spin(s.out, s.interactive, s.bofh ? tui::bofh_messages() : tui::default_messages());
    // Copy history so the thread owns its data — safe to detach after interrupt
    auto history_copy = std::make_shared<std::vector<Message>>(s.history);
    std::thread t([&s, result, done, first_token, &spin, renderer, history_copy] {
      *result = s.stream_chat(*history_copy, [first_token, &spin, renderer](const std::string& token) {
        if (g_interrupted) {
          return false;
        }
        if (!first_token->exchange(true)) {
          spin.stop();
        }
        renderer->write(token);
        return true;
      });
      renderer->finish();
      *done = true;
    });
    while (!*done && !g_interrupted) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    spin.stop();
    if (*done) {
      t.join();
    } else {
      t.detach();
    }
    // cppcheck-suppress knownConditionTrueFalse
    if (!g_interrupted && !result->empty()) {
      s.out << "\n";
    }
    return g_interrupted ? "" : *result;
  }

  // Buffered mode (mock/fallback): use spinner while waiting.
  // Same thread-safety pattern: history copied into shared_ptr.
  auto history_copy2 = std::make_shared<std::vector<Message>>(s.history);
  std::thread t([&s, result, done, history_copy2] {
    *result = s.chat(*history_copy2);
    *done = true;
  });
  while (!*done && !g_interrupted) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  if (*done) {
    t.join();
  } else {
    t.detach();
  }
  return g_interrupted ? "" : *result;
}

/// Call LLM with spinner, interruptible by Ctrl+C.
/// Installs SIGINT handler, starts spinner, calls chat on a thread.
static std::string chat_with_spinner(ReplState& s) {
  g_interrupted = 0;
  auto prev = std::signal(SIGINT, sigint_handler);
  Spinner spin(s.out, s.interactive && !s.stream_chat, s.bofh ? tui::bofh_messages() : tui::default_messages());
  std::string result = interruptible_chat(s);
  std::signal(SIGINT, prev);
  return result;
}

// --- Response handling ---
// Parses annotations from LLM response, dispatches to handlers.
// Annotations are XML-like tags: <write>, <str_replace>, <read>, <exec>, <search>.
// If no annotations found, renders the response as markdown.
// Returns true if follow-up context was added (triggers another LLM turn).

// NOLINTNEXTLINE(readability-function-size)
bool handle_response(const std::string& response, ReplState& s) {
  auto writes = parse_write_annotations(response);
  auto str_replaces = parse_str_replace_annotations(response);
  auto reads = parse_read_annotations(response);
  auto execs = parse_exec_tags(response);
  auto searches = parse_search_annotations(response);

  bool has_annotations = !writes.empty() || !str_replaces.empty() || !reads.empty() || !execs.empty() || !searches.empty();
  if (!has_annotations) {
    if (!s.stream_chat) {
      s.out << colorize_ai(tui::render_markdown(response, s.color && s.markdown), s) << "\n";
    }
    return false;
  }

  // Strip annotations and display clean text
  if (!s.stream_chat) {
    s.out << colorize_ai(tui::render_markdown(strip_exec_annotations(strip_annotations(response)), s.color && s.markdown), s) << "\n";
  }

  for (const auto& action : writes) {
    process_write(action, s.in, s.out, s.color, s.trust, s.cfg.auto_confirm_write);
  }
  for (const auto& action : str_replaces) {
    process_str_replace(action, s.in, s.out, s.color, s.trust);
  }
  bool has_followup = false;
  std::set<std::string> seen_reads;
  for (const auto& action : reads) {
    std::string key = action.path + "|" + std::to_string(action.from_line) + "-" + std::to_string(action.to_line) + "|" + action.search;
    if (!seen_reads.insert(key).second) {
      continue;
    }
    std::string ctx = process_read(action, s.out, s.color);
    if (!ctx.empty()) {
      s.history.push_back({"user", ctx});
      has_followup = true;
    }
  }
  for (const auto& cmd : execs) {
    std::string output = confirm_exec(cmd, s.cfg, s.in, s.out, s.trust);
    if (!output.empty()) {
      s.history.push_back({"user", "[command: " + cmd + "]\n" + output});
      has_followup = true;
    }
  }
  // Web search: only when enabled in config (ADR-057, privacy-first)
  if (s.cfg.allow_web_search) {
    for (const auto& action : searches) {
      std::string result = web_search(action.query, s.cfg);
      s.out << "\033[1;37m[searching: " << action.query << "]\033[0m\n";
      LOG_EVENT("repl", "web_search", action.query, result, 0, 0, 0);
      s.history.push_back({"user", result});
      has_followup = true;
    }
  }
  return has_followup;
}

// --- Prompt sending ---
// Manages the full prompt lifecycle: history, spinner, response, follow-ups.

/// Reminder injected after iteration 2 to prevent model drift (ADR-054).
static constexpr const char* reminder_nudge =
    "Reminder: be concise, only state facts about code you have read in this "
    "session, no scores without criteria.";

void send_prompt(const std::string& line, ReplState& s) {
  if (s.cfg.trace) {
    stderr_trace->log("[TRACE] iteration=%d prompt=%.50s\n", s.count, line.c_str());
  }

  // Inject reminder after iteration 2 to prevent model drift (ADR-054)
  bool inserted_reminder = false;
  if (s.count >= 2) {
    s.history.push_back({"system", reminder_nudge});
    inserted_reminder = true;
  }
  s.history.push_back({"user", line});

  // Time the LLM call for user feedback
  auto t0 = std::chrono::steady_clock::now();
  std::string response = chat_with_spinner(s);
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();

  if (g_interrupted) {
    s.out << "\n[interrupted]\n";
    s.history.pop_back();
    if (inserted_reminder) {
      s.history.pop_back();
    }
    g_interrupted = 0;
    return;
  }
  // Auto-repair malformed closing tags before parsing
  response = fix_malformed_tags(response);

  s.history.push_back({"assistant", response});
  s.last_assistant_idx = static_cast<int>(s.history.size()) - 1;
  bool needs_followup = handle_response(response, s);
  s.count++;

  // Show response stats: duration, model, response length.
  // Dim text so it doesn't distract from the actual response content.
  if (s.color) {
    s.out << "\033[2m";
  }
  if (elapsed >= 1000) {
    s.out << "[" << (elapsed / 1000) << "." << ((elapsed % 1000) / 100) << "s";
  } else {
    s.out << "[" << elapsed << "ms";
  }
  s.out << " · " << s.cfg.model;
  s.out << " · " << response.size() << " chars]";
  if (s.color) {
    s.out << "\033[0m";
  }
  s.out << "\n";

  // Follow-up loop: keep calling LLM while it produces annotations.
  // Bounded to prevent runaway turns (ADR-028).
  // Each follow-up feeds exec/read output back so the model can continue.
  constexpr int k_max_followups = 8;
  int followup_count = 0;
  while (needs_followup && followup_count < k_max_followups) {
    std::string followup = chat_with_spinner(s);
    if (g_interrupted) {
      s.out << "\n[interrupted]\n";
      g_interrupted = 0;
      break;
    }
    s.history.push_back({"assistant", followup});
    s.last_assistant_idx = static_cast<int>(s.history.size()) - 1;
    needs_followup = handle_response(followup, s);
    ++followup_count;
  }
  if (needs_followup) {
    s.out << "[follow-up limit reached]\n";
  }
}
