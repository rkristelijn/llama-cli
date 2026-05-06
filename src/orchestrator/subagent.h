/**
 * @file subagent.h
 * @brief Subagent invocation via @mention syntax (ADR-096 Phase 4).
 *
 * Parses @agentname from user input and routes to the appropriate
 * provider/subprocess. Supports @q, @opencode, @explore, etc.
 */

#ifndef ORCHESTRATOR_SUBAGENT_H
#define ORCHESTRATOR_SUBAGENT_H

#include <string>
#include <vector>

#include "ollama/ollama.h"

/// Result of parsing a @mention from user input.
struct MentionResult {
  bool has_mention = false;  ///< True if input starts with @agentname
  std::string agent;         ///< Agent name (e.g., "q", "opencode", "explore")
  std::string prompt;        ///< Remaining text after @agentname
};

/// Parse @mention from the beginning of user input.
/// Recognizes: @q, @opencode, @explore, @plan, @general, @reviewer
MentionResult parse_mention(const std::string& input);

/// Route a prompt to the named subagent and return its response.
/// Uses the appropriate provider (subprocess for q/opencode, LLM for others).
std::string invoke_subagent(const std::string& agent, const std::string& prompt, const std::vector<Message>& history);

#endif  // ORCHESTRATOR_SUBAGENT_H
