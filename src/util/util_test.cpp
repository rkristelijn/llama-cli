/**
 * @file util_test.cpp
 * @brief Unit tests for shared utility functions.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "util/util.h"

#include <doctest/doctest.h>

// --- url_encode --------------------------------------------------------------

TEST_CASE ("url_encode passes alphanumerics through") {
  CHECK (url_encode("hello123") == "hello123")
    ;
}

TEST_CASE ("url_encode encodes spaces as +") {
  CHECK (url_encode("hello world") == "hello+world")
    ;
}

TEST_CASE ("url_encode encodes special characters") {
  CHECK (url_encode("a&b=c") == "a%26b%3Dc")
    ;
  CHECK (url_encode("100%") == "100%25")
    ;
}

TEST_CASE ("url_encode preserves safe characters") {
  CHECK (url_encode("a-b_c.d~e") == "a-b_c.d~e")
    ;
}

TEST_CASE ("url_encode handles empty string") {
  CHECK (url_encode("") == "")
    ;
}

// --- color_name_to_ansi ------------------------------------------------------

TEST_CASE ("color_name_to_ansi maps standard colors") {
  CHECK (color_name_to_ansi("red") == "31")
    ;
  CHECK (color_name_to_ansi("green") == "32")
    ;
  CHECK (color_name_to_ansi("blue") == "34")
    ;
}

TEST_CASE ("color_name_to_ansi maps bright colors") {
  CHECK (color_name_to_ansi("bright-red") == "91")
    ;
  CHECK (color_name_to_ansi("bright-cyan") == "96")
    ;
}

TEST_CASE ("color_name_to_ansi maps extended colors") {
  CHECK (color_name_to_ansi("orange") == "38;5;208")
    ;
  CHECK (color_name_to_ansi("purple") == "38;5;129")
    ;
  CHECK (color_name_to_ansi("pink") == "38;5;213")
    ;
  CHECK (color_name_to_ansi("lime") == "38;5;118")
    ;
}

TEST_CASE ("color_name_to_ansi returns empty for none/default/unknown") {
  CHECK (color_name_to_ansi("none") == "")
    ;
  CHECK (color_name_to_ansi("default") == "")
    ;
  CHECK (color_name_to_ansi("nonexistent") == "")
    ;
}

// --- ansi_to_name ------------------------------------------------------------

TEST_CASE ("ansi_to_name maps standard codes") {
  CHECK (ansi_to_name("31") == "red")
    ;
  CHECK (ansi_to_name("32") == "green")
    ;
  CHECK (ansi_to_name("34") == "blue")
    ;
}

TEST_CASE ("ansi_to_name maps extended codes") {
  CHECK (ansi_to_name("38;5;208") == "orange")
    ;
  CHECK (ansi_to_name("38;5;129") == "purple")
    ;
}

TEST_CASE ("ansi_to_name returns none for unknown") {
  CHECK (ansi_to_name("99") == "none")
    ;
  CHECK (ansi_to_name("") == "none")
    ;
}

TEST_CASE ("color_name_to_ansi and ansi_to_name are inverse") {
  for (const auto& name : {"red", "green", "blue", "yellow", "cyan", "magenta", "white", "black", "orange", "purple", "pink", "lime"}) {
    CHECK (ansi_to_name(color_name_to_ansi(name)) == name)
      ;
  }
}
