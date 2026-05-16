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
#include <future>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "annotation/annotation.h"
#include "config/config.h"
#include "config/delegation.h"
#include "logging/logger.h"
#include "provider/provider_factory.h"
#include "repl/repl_annotations.h"
#include "repl/repl_commands.h"
#include "repl/repl_search.h"
#include "sync/sync.h"
#include "trace/trace.h"
#include "tui/tui.h"
#include "util/pii.h"

/// Global interrupt flag (defined in repl.cpp)
extern volatile sig_atomic_t g_interrupted;

/// SIGINT handler (defined in repl.cpp)
void sigint_handler(int);

// --- Colorize helper ---
// Re-applies AI color after any ANSI reset inside markdown rendering.

static std::string colorize_ai(const std::string& text, const ReplState& s) {
  if (!s.color) {
    return text;
  }
  // Use explicit /color ai setting if set, otherwise fall back to theme
  std::string color_code = s.ai_color.empty() ? tui::active_theme().ai.ansi() : "\033[" + s.ai_color + "m";
  if (color_code.empty()) {
    return text;
  }
  std::string result = color_code;
  std::string reset = ThemeStyle::reset();
  size_t pos = 0;
  size_t found;
  while ((found = text.find(reset, pos)) != std::string::npos) {
    result += text.substr(pos, found - pos + reset.size()) + color_code;
    pos = found + reset.size();
  }
  result += text.substr(pos) + ThemeStyle::reset();
  return result;
}

// --- Sliding window ---
// Trims history to keep system prompt + last N message pairs.
// Future: replace with context compression (backlog/019).

static void trim_history(std::vector<Message>& history, int max_pairs) {
  if (max_pairs <= 0) {
    return;
  }
  // Keep system prompt(s) at the front, trim the rest to last N*2 messages
  size_t sys_count = 0;
  while (sys_count < history.size() && history[sys_count].role == "system") {
    ++sys_count;
  }
  size_t keep = static_cast<size_t>(max_pairs) * 2;
  size_t non_sys = history.size() - sys_count;
  if (non_sys > keep) {
    history.erase(history.begin() + static_cast<long>(sys_count), history.begin() + static_cast<long>(history.size() - keep));
  }
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
    auto spin = std::make_shared<Spinner>(s.out, s.interactive,
                                          !s.spinner_name.empty() ? tui::personality_messages(s.spinner_name)
                                          : s.bofh                ? tui::personality_messages("bofh")
                                                                  : tui::default_messages());
    // Copy history and stream_chat so the thread owns its data — safe to detach
    auto history_copy = std::make_shared<std::vector<Message>>(s.history);
    auto chat_fn = s.stream_chat;
    bool pii = s.mask_pii;  // ADR-115: capture PII mask setting for thread
    std::thread t([chat_fn, result, done, first_token, spin, renderer, history_copy, pii] {
      *result = chat_fn(*history_copy, [first_token, spin, renderer, pii](const std::string& token) {
        if (g_interrupted) {
          return false;
        }
        if (!first_token->exchange(true)) {
          spin->stop();
        }
        renderer->write(pii ? mask_pii(token) : token);  // ADR-115: mask PII before render
        return true;
      });
      renderer->finish();
      *done = true;
    });
    while (!*done && !g_interrupted) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    spin->stop();
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
  // Same thread-safety pattern: chat function and history copied for safe detach.
  auto history_copy2 = std::make_shared<std::vector<Message>>(s.history);
  ChatFn chat_fn2 = s.chat;
  std::thread t([chat_fn2, result, done, history_copy2] {
    *result = chat_fn2(*history_copy2);
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
  Spinner spin(s.out, s.interactive && !s.stream_chat,
               !s.spinner_name.empty() ? tui::personality_messages(s.spinner_name)
               : s.bofh                ? tui::personality_messages("bofh")
                                       : tui::default_messages());
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
  auto delegates = parse_delegate_annotations(response);

  bool has_annotations =
      !writes.empty() || !str_replaces.empty() || !reads.empty() || !execs.empty() || !searches.empty() || !delegates.empty();
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

  bool has_followup = false;
  for (const auto& action : writes) {
    std::string reason = process_write(action, s.in, s.out, s.color, s.trust, s.cfg.auto_confirm_write);
    if (!reason.empty()) {
      s.history.push_back({"user", "[declined write to " + action.path + "] " + reason});
      has_followup = true;
    }
  }
  for (const auto& action : str_replaces) {
    std::string reason = process_str_replace(action, s.in, s.out, s.color, s.trust);
    if (!reason.empty()) {
      s.history.push_back({"user", "[declined str_replace on " + action.path + "] " + reason});
      has_followup = true;
    }
  }
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
    if (output.rfind("DECLINED:", 0) == 0) {
      // User declined with a reason — feed back to LLM
      s.history.push_back({"user", "[declined exec: " + cmd + "] " + output.substr(9)});
      has_followup = true;
    } else if (!output.empty()) {
      s.history.push_back({"user", "[command: " + cmd + "]\n" + output});
      has_followup = true;
    }
  }
  // Web search: only when enabled in config (ADR-057, privacy-first)
  if (s.cfg.allow_web_search) {
    for (const auto& action : searches) {
      std::string result = web_search(action.query, s.cfg);
      s.out << tui::active_theme().info.ansi() << "[searching: " << action.query << "]" << ThemeStyle::reset() << "\n";
      LOG_EVENT("repl", "web_search", action.query, result, 0, 0, 0);
      s.history.push_back({"user", result});
      has_followup = true;
    }
  }
  // Async delegation: LLM proposes subtask for another model (ADR-099)
  for (const auto& action : delegates) {
    // Resolve role type to concrete model via .config/delegation.yml
    auto target = resolve_delegation(action.type);
    std::string role_label = action.type.empty() ? "default" : action.type;
    std::string model_label = target.model.empty() ? Config::instance().model : target.model;
    s.out << tui::active_theme().info.ansi() << "[delegate:" << role_label << " → " << model_label << "] " << ThemeStyle::reset();
    s.out << action.prompt << "\n";
    // Confirm with user
    std::string answer;
    s.out << "Run in background? [y/n] ";
    std::getline(s.in, answer);
    if (answer == "y" || answer == "Y" || s.trust) {
      LOG_FEATURE("delegate_async");
      LOG_EVENT("repl", "delegate", action.prompt, role_label + "→" + model_label, 0, 0, 0);
      // Spawn background task — router decides which model
      const auto& task_prompt = action.prompt;
      const auto& task_target = target;
      auto future = std::async(std::launch::async, [task_prompt, task_target]() {
        Config task_cfg = Config::instance();
        if (!task_target.model.empty()) {
          task_cfg.model = task_target.model;
        }
        if (!task_target.host.empty()) {
          task_cfg.host = task_target.host;
        }
        if (!task_target.port.empty()) {
          task_cfg.port = task_target.port;
        }
        auto provider = create_provider(task_cfg);
        std::vector<Message> msgs = {{"user", task_prompt}};
        return provider->chat(msgs);
      });
      s.pending_tasks.emplace_back(PendingTask{model_label, action.prompt, std::move(future).share()});
      s.out << tui::active_theme().info.ansi() << "[task started — /tasks to check]" << ThemeStyle::reset() << "\n";
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
  // Prompt gate: validate before expensive calls (ADR-081)
  LOG_FEATURE("prompt_gate");
  std::string input = line;  // mutable copy for potential rewrite
  std::string trimmed = input;
  while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t')) {
    trimmed.erase(trimmed.begin());
  }
  if (trimmed.empty()) {
    s.out << "[empty prompt — type something or /help]\n";
    return;
  }
  // Intercept "y"/"yes"/"ja" after model suggested a command in plain text
  if ((trimmed == "y" || trimmed == "yes" || trimmed == "ja") && !s.history.empty()) {
    const auto& last = s.history.back();
    if (last.role == "assistant") {
      // Check if last response contained a backtick code block with a shell command
      auto tick = last.content.find('`');
      if (tick != std::string::npos) {
        // Extract command from `...` or ```...\n...\n```
        std::string cmd;
        if (last.content.substr(tick, 3) == "```") {
          auto start = last.content.find('\n', tick);
          auto end = last.content.find("```", tick + 3);
          if (start != std::string::npos && end != std::string::npos) {
            cmd = last.content.substr(start + 1, end - start - 1);
          }
        } else {
          auto end = last.content.find('`', tick + 1);
          if (end != std::string::npos) {
            cmd = last.content.substr(tick + 1, end - tick - 1);
          }
        }
        // Trim and validate — only single-line commands
        while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == ' ')) cmd.pop_back();
        while (!cmd.empty() && (cmd.front() == '\n' || cmd.front() == ' ')) cmd.erase(cmd.begin());
        if (!cmd.empty() && cmd.find('\n') == std::string::npos) {
          s.out << "[running: " << cmd << "]\n";
          input = "!" + cmd;  // Treat as shell exec
        }
      }
    }
  }
  // Warn on very short prompts (likely incomplete thought)
  int words = 1;
  for (char c : trimmed) {
    if (c == ' ') {
      words++;
    }
  }
  if (words < 3 && trimmed.back() != '?') {
    s.out << tui::active_theme().warning.ansi() << "[hint: short prompt — try adding more context]" << ThemeStyle::reset() << "\n";
  }
  // Detect repeat of previous prompt
  if (s.history.size() >= 2) {
    for (int i = static_cast<int>(s.history.size()) - 1; i >= 0; i--) {
      if (s.history[i].role == "user" && s.history[i].content == input) {
        s.out << tui::active_theme().warning.ansi() << "[hint: same as a previous prompt — rephrase?]" << ThemeStyle::reset() << "\n";
        break;
      }
    }
  }

  if (s.cfg.trace) {
    stderr_trace->log("[TRACE] iteration=%d prompt=%.50s\n", s.count, input.c_str());
  }

  // Inject reminder after iteration 2 to prevent model drift (ADR-054)
  bool inserted_reminder = false;
  if (s.count >= 2) {
    s.history.push_back({"system", reminder_nudge});
    inserted_reminder = true;
  }
  s.history.push_back({"user", input});

  // Sliding window: trim old messages before sending to LLM
  trim_history(s.history, s.cfg.max_history);

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
    s.out << tui::active_theme().system.ansi();
  }
  if (elapsed >= 1000) {
    s.out << "[" << (elapsed / 1000) << "." << ((elapsed % 1000) / 100) << "s";
  } else {
    s.out << "[" << elapsed << "ms";
  }
  s.out << " · " << Config::instance().model;
  s.out << " · " << response.size() << " chars]";
  if (s.color) {
    s.out << ThemeStyle::reset();
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

  // Show periodic tips to help users discover commands
  show_tip(s);
}
