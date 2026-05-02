/**
 * @file repl_annotations.h
 * @brief Annotation handlers for REPL mode (write/read/exec/str_replace).
 *
 * SRP: Handles all LLM-proposed file and command actions in interactive mode.
 * Extracted from repl.cpp to reduce file size (ADR-061, ADR-065).
 *
 * @see docs/adr/adr-014-tool-annotations.md
 * @see docs/adr/adr-019-safe-file-writes.md
 */

#ifndef REPL_ANNOTATIONS_H
#define REPL_ANNOTATIONS_H

#include <iostream>
#include <string>

#include "annotation/annotation.h"
#include "config/config.h"

// --- Diff display ---

/// Print a git-style unified diff between old and new text with ANSI colors.
void show_diff(const std::string& old_text, const std::string& new_text, std::ostream& out, bool color);

// --- Annotation action handlers ---

/// Prompt user and write a file if confirmed (backup existing, show diff).
void process_write(const WriteAction& action, std::istream& in, std::ostream& out, bool color, bool& trust, bool auto_confirm);

/// Prompt user and apply a str_replace if confirmed (show diff, backup).
void process_str_replace(const StrReplaceAction& action, std::istream& in, std::ostream& out, bool color, bool& trust);

/// Execute a <read> action and return context string for the LLM.
std::string process_read(const ReadAction& action, std::ostream& out, bool color);

/// Strip <exec> annotations from text, replacing with readable placeholders.
std::string strip_exec_annotations(const std::string& text);

/// Confirm and execute an LLM-proposed command (ADR-015).
/// Returns command output if executed, empty string if declined.
std::string confirm_exec(const std::string& cmd, const Config& cfg, std::istream& in, std::ostream& out, bool& trust);

#endif
