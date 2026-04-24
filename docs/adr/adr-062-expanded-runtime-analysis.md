---
title: "ADR-062: Expanded Runtime Analysis — TSan, Valgrind, IWYU"
summary: Add ThreadSanitizer, Valgrind, and include-what-you-use to CI to close remaining blind spots
status: accepted
date: 2026-04-24
---

## Status

Accepted

## Context

Current checks miss three categories of bugs:

| Bug type | Example | Current detection | Gap |
|----------|---------|-------------------|-----|
| Data races | Spinner thread + main thread share state | None | **TSan** |
| Memory leaks | Allocated but never freed | ASan (partial) | **Valgrind** |
| Uninitialized reads | Reading stack variable before assignment | ASan (no) | **Valgrind/MSan** |
| Unnecessary includes | Slow builds, hidden coupling | None | **IWYU** |

A use-after-free slipped past all existing checks in a prior PR, confirming that ASan alone is insufficient.

## Decision

### 1. ThreadSanitizer (TSan) — CI job

Detects data races in threaded code (Spinner, streaming chat, interruptible_chat).

```bash
cmake -B build -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
cmake --build build
./build/test_repl    # threading code
./build/test_ollama  # streaming mock
```

Cannot combine with ASan — separate CI job required.

### 2. Valgrind — CI job

Detects memory leaks and uninitialized reads. Slow (~10x) but thorough.

```bash
sudo apt-get install -y valgrind
valgrind --leak-check=full --error-exitcode=1 ./build/test_repl
```

Linux-only (macOS support is poor). Run on a subset of test binaries to keep CI under 5 minutes.

### 3. include-what-you-use (IWYU) — optional/manual

Finds missing and unnecessary `#include` directives. Useful during the ADR-061 file splits.

```bash
cmake -B build -DCMAKE_CXX_INCLUDE_WHAT_YOU_USE=include-what-you-use
cmake --build build 2>&1 | grep "should add\|should remove"
```

Not a CI gate — run manually before large refactors.

## Also: fix sanitizer coverage

The current sanitizer CI job only runs 7 of 15 test binaries. Update to run all:

```bash
for t in build/test_*; do ./$t; done
```

## Consequences

- TSan catches threading bugs before they reach users
- Valgrind catches leaks that ASan misses
- Both run in CI only — no impact on local `make check` speed
- IWYU supports the modularization effort (ADR-061)
