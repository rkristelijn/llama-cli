// Network scanner — discovers Ollama servers on the local subnet.
// Uses parallel TCP connection attempts with short timeouts.

#ifndef SCAN_H
#define SCAN_H

#include <string>
#include <vector>

/// Scan local subnet for Ollama servers (port 11434 by default).
/// Returns list of "host:port" strings that responded to /api/tags.
std::vector<std::string> scan_ollama_hosts(int port = 11434);

/// Get the local machine's subnet prefix (e.g. "192.168.1.")
std::string get_local_subnet();

#endif
