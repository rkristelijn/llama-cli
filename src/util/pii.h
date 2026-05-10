/// @file pii.h
/// @brief PII masking utility — replaces sensitive data in output (ADR-115)
#ifndef LLAMA_CLI_PII_H
#define LLAMA_CLI_PII_H

#include <string>

/// Mask PII patterns in text: IPv4, IPv6, hostname, home path, email, API keys.
/// Uses environment ($HOSTNAME, $HOME, $USER) for dynamic patterns.
std::string mask_pii(const std::string& text);

#endif  // LLAMA_CLI_PII_H
