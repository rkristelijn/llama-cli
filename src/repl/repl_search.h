/**
 * @file repl_search.h
 * @brief Web search integration for the REPL (ADR-057).
 *
 * SRP: Docker/SearXNG lifecycle and web search queries.
 * Extracted from repl.cpp to reduce file size (ADR-065).
 */

#ifndef REPL_SEARCH_H
#define REPL_SEARCH_H

#include <iostream>
#include <string>

#include "config/config.h"

/// Ensure SearXNG is running (starts Docker + container if needed).
/// No-op if already responding. Outputs status messages to out.
void ensure_searxng(const Config& cfg, std::ostream& out);

/// Search the web via SearXNG JSON API. Returns formatted results string.
/// Returns "[web search failed]" on error, "[web search: no results...]" if empty.
std::string web_search(const std::string& query, const Config& cfg);

#endif
