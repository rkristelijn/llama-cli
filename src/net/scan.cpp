/**
 * @file scan.cpp
 * @brief Network scanner for Ollama servers on the local subnet.
 *
 * Scans all 254 IPs on the local subnet in parallel using threads.
 * Each thread attempts a TCP connection + HTTP GET /api/tags with a
 * short timeout. Hosts that respond with valid JSON are collected.
 */

#include "net/scan.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>

#include <mutex>
#include <thread>
#include <vector>

/// @brief Override httplib connect timeout to 1 second for fast subnet scanning.
#define CPPHTTPLIB_CONNECT_TIMEOUT 1
#include <httplib.h>

/// Detect the local subnet prefix from network interfaces.
/// Picks the first non-loopback IPv4 interface (e.g. "192.168.1.").
std::string get_local_subnet() {
  struct ifaddrs* addrs = nullptr;
  if (getifaddrs(&addrs) != 0) {
    return "192.168.1.";  // fallback
  }
  std::string result;
  for (auto* ifa = addrs; ifa != nullptr; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr) continue;
    if (ifa->ifa_addr->sa_family != AF_INET) continue;
    // Skip loopback
    if (ifa->ifa_flags & IFF_LOOPBACK) continue;
    auto* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf));
    std::string ip(buf);
    // Extract subnet prefix (first 3 octets)
    auto last_dot = ip.rfind('.');
    if (last_dot != std::string::npos) {
      result = ip.substr(0, last_dot + 1);
      break;
    }
  }
  freeifaddrs(addrs);
  return result.empty() ? "192.168.1." : result;
}

/// Scan the local subnet for Ollama servers.
/// Spawns threads to probe each IP in parallel (1s connect timeout).
std::vector<std::string> scan_ollama_hosts(int port) {
  std::string subnet = get_local_subnet();
  std::vector<std::string> found;
  std::mutex mtx;
  std::vector<std::thread> threads;

  // Probe each IP on the subnet
  for (int i = 1; i <= 254; i++) {
    std::string ip = subnet + std::to_string(i);
    threads.emplace_back([ip, port, &found, &mtx]() {
      std::string url = "http://" + ip + ":" + std::to_string(port);
      httplib::Client cli(url);
      cli.set_connection_timeout(1, 0);
      cli.set_read_timeout(1, 0);
      auto res = cli.Get("/api/tags");
      if (res && res->status == 200 && res->body.find("models") != std::string::npos) {
        std::lock_guard<std::mutex> lock(mtx);
        found.push_back(ip + ":" + std::to_string(port));
      }
    });
  }

  // Wait for all probes to complete
  for (auto& t : threads) {
    t.join();
  }
  return found;
}
