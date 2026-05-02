
---

title: ADR-069: Memory Safety Verification Strategy
date: 2026-04-29
status: accepted
deciders: Remi Kristelijn

## tags: [security, testing, memory-safety, c++]

---

# ADR-069: Memory Safety Verification Strategy

## Context

C++ provides manual memory management, which introduces the risk of memory-related vulnerabilities, specifically **Use-After-Free (UAF)**, **Double Free**, and **Dangling Pointers**.

As `llama-cli` handles external command execution and complex string parsing (annotations, JSON, etc.), a single memory error could lead to a crash or, in a worst-case scenario, an exploitable security vulnerability.

While LLMs (like CodeRabbit) can identify patterns of unsafe code through semantic analysis, they are probabilistic and prone to false negatives/positives. We need a deterministic, engineering-led approach to ensure memory safety.

## Problem Statement

How can we reliably detect and prevent memory-related errors (specifically UAF) within the `llama-cli` development lifecycle?

## Decision Drivers

* **Reliability:** We need deterministic detection (not probabilistic AI).
* **Performance:** Verification should not significantly slow down the `make quick` feedback loop.
* **Developer Experience:** Tools should integrate into the existing `Makefile` and CI pipeline.
* **Platform Coverage:** Support for both Linux and macOS (primary dev environments).

## Considered Options

### 1. Static Analysis Only (Clang-Tidy / Cppcheck)

Checking code patterns without executing it.

* **Pros:** Extremely fast; no execution required; catches errors during `make lint`.
* **Cons:** Cannot detect "temporal" errors (errors that only happen when specific logic paths are executed at runtime).

### 2. Dynamic Analysis (AddressSanitizer - ASan)

Instrumenting the binary to monitor memory access at runtime.

* **Pros:** Extremely high precision; catches real UAF and buffer overflows as they happen; provides exact stack traces.
* **Cons:** Requires recompilation; adds runtime overhead.

### 3. Runtime Heap Monitoring (Windows Debug Heap / Page Heap)

Using OS-level features to detect heap corruption.

* **Pros:** Useful for closed-source binaries or external dependencies.
* **Cons:** Platform-specific (Windows-centric); harder to integrate into a cross-platform Linux/macOS workflow.

## Decision Outcome

Chosen Option: **Hybrid Strategy (Static + Dynamic)**

We will implement a multi-layered approach:

1. **Layer 1 (Prevention - Static):** Continue using `clang-tidy` and `cppcheck` in the `make lint` and `cache` targets to catch obvious lifecycle mismanagement (e.g., uninitialized pointers) via AST analysis.
2. **Layer 2 (Detection - Dynamic):** Mandate the use of **AddressSanitizer (ASan)** for all unit tests (`make test-unit`) and end-to-end tests (`make e2e`).
   * The `Makefile` will allow enabling ASan via a flag (e.g., `make test-unit SANITIZE=address`).
   * This ensures that any UAF triggered by the logic of the REPL or Command Execution is caught immediately during the CI pipeline.
3. **Layer 3 (Development Practice):** Enforce the "Safe Pointer" pattern: always set pointers to `nullptr` immediately after `delete` (as noted in `TODO.md` discussions) to turn UAF into a predictable null-pointer dereference.

## Consequences

* **Positive:** High confidence in memory safety; automated detection of complex temporal bugs; zero manual effort once integrated into `make test`.
* **Negative:** Increased build time for the `test` target due to instrumentation overhead.
* **Neutral:** Requires developers to be aware of ASan logs when debugging test failures.
