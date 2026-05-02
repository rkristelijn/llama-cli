/**
 * @file sync_annotations.h
 * @brief Annotation processing for sync (non-interactive) mode.
 *
 * SRP: Handles all LLM-proposed file and command actions in sync mode
 * (no user confirmation — auto-applies within sandbox).
 * Extracted from main.cpp to reduce file size (ADR-061, ADR-065).
 *
 * @see docs/adr/adr-030-stdin-sync.md
 */

#ifndef SYNC_ANNOTATIONS_H
#define SYNC_ANNOTATIONS_H

#include <string>

#include "config/config.h"

/// Process all annotations in a sync-mode LLM response.
/// Applies read/exec/write/str_replace/add_line/delete_line actions
/// within the configured sandbox. Returns followup context for the LLM.
std::string process_sync_annotations(const std::string& response, const Config& cfg);

#endif
