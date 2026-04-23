/**
 * @file fuzz_annotation.cpp
 * @brief libFuzzer target for annotation parsing.
 *
 * Feeds random/mutated strings into all annotation parsers to find
 * crashes, hangs, and undefined behavior in LLM response parsing.
 *
 * Build: cmake -B build -DENABLE_FUZZ=ON && cmake --build build --target fuzz_annotation
 * Run:   ./build/fuzz_annotation -max_total_time=60
 */

#include <cstdint>
#include <string>

#include "annotation/annotation.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::string input(reinterpret_cast<const char*>(data), size);

  // Exercise all annotation parsers with arbitrary input
  parse_write_annotations(input);
  parse_str_replace_annotations(input);
  parse_read_annotations(input);
  strip_annotations(input);

  return 0;
}
