/**
 * @file delegation.h
 * @brief Role-based delegation router (ADR-100).
 *
 * Maps delegation types ("code", "research", "review") to concrete
 * model\@host:port targets. Config loaded from .config/delegation.yml.
 */

#ifndef DELEGATION_H
#define DELEGATION_H

#include <string>

/// Resolved delegation target: which model on which host.
struct DelegationTarget {
  std::string model;  ///< Model name (e.g. "qwen2.5-coder:14b")
  std::string host;   ///< Host (empty = use current config)
  std::string port;   ///< Port (empty = use current config)
};

/// Resolve a delegation role type to a concrete model/host.
/// Reads .config/delegation.yml on first call, caches result.
/// Returns empty model if type is unknown.
DelegationTarget resolve_delegation(const std::string& type);

#endif  // DELEGATION_H
