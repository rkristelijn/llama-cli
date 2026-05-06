# ADR-097: C++ Code Quality Checks and Best Practices

**Status**: Accepted  
**Date**: 2026-05-06  
**Deciders**: Development Team  
**Context**: Analysis of common C++ bugs from Reddit r/cpp, PVS-Studio Top 10 2024, and C++ Core Guidelines

## Context

After analyzing common C++ problems from industry discussions (Reddit r/cpp), real-world bugs found by static analyzers (PVS-Studio Top 10 2024), and the C++ Core Guidelines, we identified critical gaps in our current quality checks.

### Current State

**Existing checks** (good):

- `-Wall -Wextra -Werror` compiler flags
- Clang-tidy with basic checks
- ASAN/UBSAN in CI
- Static analysis (Semgrep, Gitleaks)
- Code formatting (clang-format)

**Missing checks** (gaps):

- No `-Wconversion` flag (implicit conversions)
- No lifetime analysis (dangling references)
- No Unicode validation
- No override enforcement
- No unreachable code detection

## Decision

Implement the following checks based on real-world bug analysis:

### Priority 0 (Critical - Immediate)

#### 1. Implicit Conversions

**Source**: Reddit r/cpp - "caused horrific bugs in production"  
**Impact**: Data loss in production

**Found issues**:

- `src/json/json.cpp:57` - `int` → `size_t` conversion
- `src/json/json.cpp:143` - `int` → `size_t` conversion

**Implementation**:

```cmake
add_compile_options(-Wconversion -Wsign-conversion)
```

**Script**: `scripts/lint/check-conversions.sh` ✅

#### 2. Virtual Function Override

**Source**: PVS-Studio Bug #8 (Blender)  
**Impact**: Silent function mismatch due to namespace issues

**Implementation**:

```cmake
add_compile_options(-Werror=suggest-override)
```

**Clang-tidy**:

```yaml
modernize-use-override,
hicpp-use-override
```

#### 3. Invisible Unicode Characters

**Source**: PVS-Studio Bug #4 (Godot Engine)  
**Impact**: Security backdoor (ZERO WIDTH SPACE U+200B)

**Implementation**: `scripts/lint/check-unicode.sh` ✅

Detects:

- U+200B ZERO WIDTH SPACE
- U+200C ZERO WIDTH NON-JOINER
- U+200D ZERO WIDTH JOINER
- U+FEFF ZERO WIDTH NO-BREAK SPACE

### Priority 1 (High - Short Term)

#### 4. Integer Division Bugs

**Source**: PVS-Studio Bug #3 (Xenia)  
**Impact**: Always-zero results in calculations

**Implementation**:

```yaml
Checks: bugprone-integer-division
```

#### 5. Unreachable Code

**Source**: PVS-Studio Bug #7 (Qt Creator - 14 years old!)  
**Impact**: Dead code, potential logic errors

**Implementation**:

```cmake
add_compile_options(-Wunreachable-code)
```

**Clang-tidy**:

```yaml
clang-analyzer-deadcode.DeadStores
```

#### 6. Memory Leaks (realloc)

**Source**: PVS-Studio Bug #10 (PPSSPP)  
**Impact**: Memory leak when realloc fails

**Pattern**:

```cpp
// ❌ Wrong
hello = realloc(hello, size);

// ✅ Correct
void* tmp = realloc(hello, size);
if (tmp) hello = tmp;
```

**Implementation**:

```yaml
Checks: bugprone-suspicious-realloc-usage
```

### Priority 2 (Medium - Long Term)

#### 7. Lifetime Analysis

**Source**: Reddit r/cpp - `std::string_view` outlives `std::string`

**Implementation**:

```yaml
Checks: bugprone-dangling-handle
```

Compiler flag (Clang 16+):

```cmake
add_compile_options(-Wlifetime)
```

#### 8. Array Bounds / Enum Mismatch

**Source**: PVS-Studio Bug #1 (DPDK) - Most dangerous

**Pattern**:

```cpp
enum Status { A, B, C, MAX };
const char* names[] = { "A", "B" };  // ❌ Missing "C"

// ✅ Fix with compile-time check
static_assert(std::size(names) == MAX, "Array size mismatch");
```

#### 9. Duplicate Map Keys

**Source**: PVS-Studio Bug #9 (OpenVINO)

**Pattern**:

```cpp
std::unordered_map<std::string, Type> map = {
    {"Abs", Type::Eltwise},
    {"Abs", Type::Math},  // ❌ Duplicate - second never used
};
```

**Detection**: Clang-tidy V766 equivalent

#### 10. Null Pointer Dereference

**Source**: PVS-Studio Bug #2 (LLVM)

**Current**: ASAN (already active) ✅

**Additional**:

```yaml
Checks: clang-analyzer-core.NullDereference
```

#### 11. Slicing Problem (Pass by Value)

**Source**: Toptal - Common Mistake #9

**Pattern**:

```cpp
class A { virtual std::string GetName() const {return "A";} };
class B: public A { virtual std::string GetName() const {return "B";} };

void func(A a) { a.GetName(); }  // ❌ Slices B to A
B b;
func(b);  // Calls A::GetName(), not B::GetName()
```

**Implementation**:

```yaml
Checks: performance-unnecessary-value-param
```

#### 12. Explicit Constructors

**Source**: Toptal - Common Mistake #10

**Pattern**:

```cpp
class String {
    String(int n);  // ❌ Allows implicit conversion
};
String s = 123;  // ❌ Creates string of size 123, not "123"
```

**Implementation**:

```yaml
Checks: google-explicit-constructor,
        hicpp-explicit-conversions
```

#### 13. Exception Safety in Destructors

**Source**: Toptal - Common Mistake #6

**Pattern**:

```cpp
~A() {
    writeToLog();  // ❌ Can throw, causes terminate()
}
```

**Implementation**:

```yaml
Checks: bugprone-exception-escape
```

#### 14. Integer Overflow/Wraparound

**Source**: CWE-190 (Code Intelligence Top 6) - 3 KEV exploits

**Pattern**:

```cpp
int max = INT_MAX;
int result = max + 1;  // ❌ Undefined behavior
```

**Implementation**:

```cmake
# Compiler flag
add_compile_options(-ftrapv)  # Trap on signed overflow

# Or use sanitizer
add_compile_options(-fsanitize=integer)
```

**Clang-tidy**:

```yaml
Checks: bugprone-narrowing-conversions,
        cert-int30-c,
        cert-int31-c
```

**Note**: Out-of-bounds (CWE-787, CWE-125), use-after-free (CWE-416), buffer overflow (CWE-119), and null pointer (CWE-476) are already covered by ASAN ✅

#### 15. Copy-Paste Errors

**Source**: PVS-Studio Bug #6 (Telegram)

**Pattern**:

```cpp
const auto allow = v::is<TypeA>(_source)
                || v::is<TypeA>(_source)  // ❌ Copy-paste, should be TypeB
                || other_check;
```

**Implementation**:

```yaml
Checks: misc-redundant-expression
```

**Note**: Also caught by duplicate code detection (jscpd) ✅

#### 16. Preprocessor Directive Typos

**Source**: PVS-Studio Bug #5 (qdEngine)

**Pattern**:

```cpp
#ifndef _QUEST_EDITOR
    // code
else           // ❌ Should be #else
    // code
#endif
```

**Detection**: Compiler warnings + code review

**Note**: Modern C++ should minimize preprocessor usage

#### 17. Variable Shadowing

**Source**: C++ Core Guidelines ES.12

**Pattern**:

```cpp
int x = 10;
{
    int x = 20;  // ❌ Shadows outer x
    use(x);      // Which x? Confusing!
}
```

**Implementation**:

```cmake
add_compile_options(-Wshadow)
```

**Script**: `scripts/lint/check-shadowing.sh`  
**Autofix**: `scripts/lint/fix-shadowing.sh` (manual guidance)

**Rationale**: Prevents confusion and bugs from accidentally using wrong variable

#### 18. C-Style Casts

**Source**: C++ Core Guidelines ES.48

**Pattern**:

```cpp
void* ptr = malloc(100);
int* data = (int*)ptr;  // ❌ C-style cast

// ✅ Use C++ casts
int* data = static_cast<int*>(ptr);
```

**Implementation**:

```cmake
add_compile_options(-Wold-style-cast)
```

**Script**: `scripts/lint/check-casts.sh`  
**Autofix**: `scripts/lint/fix-casts.sh` ✅

**Rationale**: C++ casts are more explicit and searchable

#### 19. Null Pointer Literals

**Source**: C++ Core Guidelines ES.47

**Pattern**:

```cpp
int* ptr = 0;      // ❌ Old style
int* ptr = NULL;   // ❌ C style
int* ptr = nullptr;  // ✅ C++11

void func(int);
void func(int*);
func(0);       // ❌ Calls func(int), not func(int*)
func(nullptr); // ✅ Calls func(int*)
```

**Implementation**:

```yaml
Checks: modernize-use-nullptr
```

**Autofix**: `scripts/lint/fix-nullptr.sh` ✅

**Rationale**: Type-safe, prevents overload resolution bugs

## Implementation Plan

### Phase 1 (Immediate)

```bash
# 1. Add to CMakeLists.txt
add_compile_options(-Wconversion -Wsign-conversion)
add_compile_options(-Werror=suggest-override)
add_compile_options(-Wunreachable-code)

# 2. Fix existing issues
./scripts/lint/check-conversions.sh
# Fix src/json/json.cpp:57, 143

# 3. Add to CI
./scripts/lint/check-unicode.sh
```

### Phase 2 (Short Term)

```yaml
# Update .config/.clang-tidy
Checks: >
  ...,
  modernize-use-override,
  hicpp-use-override,
  bugprone-integer-division,
  bugprone-suspicious-realloc-usage,
  clang-analyzer-deadcode.DeadStores,
  performance-unnecessary-value-param,
  google-explicit-constructor,
  hicpp-explicit-conversions,
  bugprone-exception-escape,
  misc-redundant-expression
```

### Phase 3 (Long Term)

- Add `bugprone-dangling-handle` for lifetime analysis
- Investigate `-Wlifetime` (Clang 16+)
- Add `static_assert` for enum/array alignment

## Consequences

### Positive

- Catch data loss bugs before production
- Prevent security backdoors (Unicode)
- Enforce modern C++ practices (override)
- Reduce code review burden

### Negative

- Initial effort to fix existing issues (2 conversions in json.cpp)
- Some false positives possible (tune with pragmas)
- Build time slightly increased

### Neutral

- Developers must use explicit casts where needed
- More verbose code in some cases

## Validation

### Test Results

```bash
$ ./scripts/lint/check-conversions.sh
❌ Found implicit conversion errors:
src/json/json.cpp:57:68: error: conversion to 'size_t' from 'int'
src/json/json.cpp:143:14: error: conversion to 'size_t' from 'int'

$ ./scripts/lint/check-unicode.sh
✅ No invisible Unicode characters found
```

### Real-World Evidence

All bugs are from production open-source projects:

- Blender (3D graphics)
- LLVM (compiler infrastructure)
- Telegram (messaging)
- Godot Engine (game engine)
- DPDK (data plane development kit)

## References

- Reddit r/cpp: "what are some common C++ problems you keep seeing"
- PVS-Studio: "Top 10 most notorious C and C++ errors in 2024"
- Toptal: "Top 10 Most Common C++ Mistakes That Developers Make"
- Code Intelligence: "Top Six Most Dangerous Vulnerabilities in C and C++" (CWE Top 25)
- C++ Core Guidelines: ES.12 (shadowing), ES.47 (nullptr), ES.48 (casts)
- CWE-190: Integer Overflow or Wraparound
- CWE-561: Dead Code
- CWE-704: Incorrect Type Conversion

## Related ADRs

- ADR-002: Quality Checks
- ADR-048: Quality Framework
- ADR-053: C++ Static Analysis

## Notes

Quote from Reddit r/cpp:
> "Accidental conversions that truncate values have caused so many horrific bugs in production. I mostly work in database engines and storage code these days, so I have seen real production data loss issues caused by implicit conversion bugs. My first order of business in any project now is to add -Werror -Wconversion."

The PVS-Studio Bug #1 (array mismatch) went undetected for years despite code reviews. Static analyzers are essential.
