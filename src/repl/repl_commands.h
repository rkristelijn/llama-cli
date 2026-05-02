/**
 * @file repl_commands.h
 * @brief Slash command handlers extracted from repl.cpp (SRP).
 *
 * SRP: slash commands live here so repl.cpp only handles the input loop.
 * ISP: only dispatch_command() is needed by repl.cpp — internal helpers stay private.
 *
 * @see docs/adr/adr-066-solid-refactoring.md
 */

#ifndef LLAMA_CLI_REPL_COMMANDS_H
#define LLAMA_CLI_REPL_COMMANDS_H

#include <iostream>
#include <string>
#include <vector>

#include "config/config.h"
#include "exec/hardware.h"
#include "net/scan.h"
#include "ollama/ollama.h"
#include "repl/repl.h"

/// REPL session state — groups related data to reduce parameter passing.
/// Shared between repl.cpp (loop) and repl_commands.cpp (handlers).
struct ReplState {
  ChatFn& chat;                     ///< Injected chat function (real or mock)
  StreamChatFn stream_chat;         ///< Streaming chat function (nullable)
  ModelsFn& models_fn;              ///< Injected model fetcher (real or mock)
  ModelInfoFn model_info_fn;        ///< Injected model info fetcher (real or mock)
  HardwareFn hw_fn;                 ///< Injected hardware detector (real or mock)
  ScanFn scan_fn;                   ///< Injected network scanner (real or mock)
  const Config& cfg;                ///< Configuration (timeouts, etc.)
  std::vector<Message>& history;    ///< Conversation history
  std::istream& in;                 ///< Input stream
  std::ostream& out;                ///< Output stream
  int count = 0;                    ///< Number of prompts processed
  bool color = false;               ///< Whether to use ANSI colors (TTY detect)
  bool interactive = false;         ///< Whether running on a real TTY (for spinner)
  bool markdown = true;             ///< Whether to render markdown in LLM output
  bool bofh = false;                ///< BOFH mode: sarcastic spinner
  bool warmup = false;              ///< Whether to warm up model on switch
  std::string prompt_color = "32";  ///< ANSI code for user prompt (green)
  std::string ai_color = "";        ///< ANSI code for AI response (none=default)
  bool trust = false;               ///< Trust mode: auto-approve all actions
  int last_assistant_idx = -1;      ///< Index of last assistant message for rating
};

/// OCP: dispatch_command routes to handlers — new commands add new functions,
/// not edits to existing ones.
bool dispatch_command(const std::string& command, const std::string& arg, ReplState& s);

#endif  // LLAMA_CLI_REPL_COMMANDS_H
