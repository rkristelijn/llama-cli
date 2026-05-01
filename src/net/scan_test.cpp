/**
 * @file scan_test.cpp
 * @brief Unit tests for network scanner (get_local_subnet, scan_ollama_hosts).
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "net/scan.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <string>

TEST_CASE ("get_local_subnet returns a valid subnet prefix") {
  std::string subnet = get_local_subnet();
  CHECK_FALSE (subnet.empty())
    ;
  // Must end with a dot (e.g. "192.168.1.")
  CHECK (subnet.back() == '.')
    ;
  // Must have exactly 3 dots (3 octets + trailing dot)
  auto dots = std::count(subnet.begin(), subnet.end(), '.');
  CHECK (dots == 3)
    ;
}

TEST_CASE ("get_local_subnet returns consistent results") {
  // Calling twice should return the same subnet
  std::string a = get_local_subnet();
  std::string b = get_local_subnet();
  CHECK (a == b)
    ;
}

TEST_CASE ("scan_ollama_hosts returns empty for unused port") {
  // Port 1 is almost certainly not running Ollama
  auto hosts = scan_ollama_hosts(1);
  CHECK (hosts.empty())
    ;
}
