/**
 * @file repl_model.h
 * @brief Interactive model selection UI for the REPL.
 *
 * SRP: Model listing, sorting, hardware display, and selection prompt.
 * Extracted from repl_commands.cpp to reduce file size (ADR-065).
 *
 * @see docs/adr/adr-049-model-selection-command.md
 */

#ifndef REPL_MODEL_H
#define REPL_MODEL_H

#include <string>

#include "repl/repl_commands.h"

/// Interactive model selection: shows table, accepts sort/pick input.
void handle_model_selection(ReplState& s, const std::string& arg);

#endif
