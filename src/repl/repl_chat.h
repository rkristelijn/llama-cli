/**
 * @file repl_chat.h
 * @brief Chat loop logic: send prompts, handle responses, manage follow-ups.
 *
 * SRP: LLM interaction (spinner, streaming, annotation dispatch, follow-ups).
 * Extracted from repl.cpp to reduce file size (ADR-065).
 */

#ifndef REPL_CHAT_H
#define REPL_CHAT_H

#include <string>

#include "repl/repl_commands.h"

/// Send a user prompt to the LLM, handle response and follow-ups.
/// Manages spinner, SIGINT, history, annotations, and bounded follow-up loop.
void send_prompt(const std::string& line, ReplState& s);

/// Process an LLM response: render text, dispatch annotations.
/// Returns true if follow-up context was added (exec output, read result).
bool handle_response(const std::string& response, ReplState& s);

#endif
