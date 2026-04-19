# ADR-053: C++ Static Analysis Coverage

## Status

Proposed

## Date

2026-04-19

## Context

We run clang-tidy, cppcheck, ASan/UBSan, and `-Wall -Wextra -Werror` but still miss several common C++ bug categories. This ADR maps the most frequent C++ mistakes to our current tooling and identifies gaps.

## Current coverage

| Bug category | Covered | Tool |
|---|---|---|
| Memory leaks | ✅ | ASan (CI sanitizer job) |
| Use-after-free / dangling pointers | ✅ | ASan + bugprone-use-after-move |
| Buffer overflow | ✅ | ASan + cppcheck |
| Integer overflow / division by zero | ✅ | UBSan + bugprone-integer-division |
| Assignment in condition (`=` vs `==`) | ✅ | -Wall (-Wparentheses) |
| sizeof misuse | ✅ | bugprone-sizeof-expression |
| Uninitialized variables | ⚠️ | -Wall catches most, not all |
| Null pointer dereference | ⚠️ | ASan runtime only, no static check |
| Unnecessary copies | ❌ | Not checked |
| Missing const | ❌ | Not checked |
| Implicit narrowing conversions | ❌ | -Wconversion not enabled |
| Raw new/delete (vs smart pointers) | ❌ | Not checked (not used currently) |
| Exception safety | ❌ | Not checked |

## Decision

Add the following clang-tidy checks to `.config/.clang-tidy`:

### Performance (unnecessary copies)

- `performance-unnecessary-copy-initialization` — detects copies that could be const refs
- `performance-for-range-copy` — range-for loops that copy instead of reference
- `performance-unnecessary-value-param` — function params passed by value that should be const ref
- `performance-move-const-arg` — std::move on const or trivial types

### Bugprone (additional)

- `bugprone-dangling-handle` — string_view/span pointing to destroyed temporaries
- `bugprone-copy-constructor-init` — copy constructors that don't initialize base classes
- `bugprone-bool-pointer-implicit-conversion` — bool* accidentally used as bool

### Not adding (rationale)

- `-Wconversion` — too noisy for our codebase (size_t/int conversions everywhere), revisit later
- `modernize-use-auto` — conflicts with "simple C++ for beginners" philosophy
- `cppcoreguidelines-owning-memory` — we don't use raw new/delete, not needed yet
- `misc-const-correctness` — clang-tidy 16+ only, not available on CI ubuntu-24.04

## Consequences

- `make tidy` catches more real bugs with minimal false positives
- Performance checks prevent accidental O(n) copies in hot paths
- No changes to compiler flags (no -Wconversion yet)

## References

- @see `.config/.clang-tidy`
- @see ADR-048 (quality framework)
