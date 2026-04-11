# ADR-008: Test Framework

*Status*: Accepted · *Date*: 2026-04-10 · *Context*: Raw `assert()` tests lack BDD-style structure and mocking support. A test framework is needed to improve readability and enable HTTP call mocking.

## Options Considered

| Framework | Style | Mocking | Size | Header-only |
|-----------|-------|---------|------|-------------|
| **GoogleTest** | `TEST`/`TEST_F` | GMock built-in | Large | No |
| **Catch2** | `TEST_CASE`/`SECTION` | Separate lib needed | Medium | Yes |
| **doctest** | `TEST_CASE`/`SUBCASE` | Separate lib needed | Smallest | Yes |

## Decision
**doctest** was chosen as the test framework. HTTP mocking is handled via function pointer injection rather than a mock library.

### Why not GoogleTest?
- GMock is powerful but heavy — overkill for the current codebase
- Longer compile times conflict with the lean approach

### Why not Catch2?
- Larger than doctest with no significant advantage for this use case
- Mocking still requires a separate library

### Mocking approach
Instead of a mock framework, the Ollama client is abstracted behind a function signature. Tests inject a fake implementation:

```cpp
// Production: real HTTP call
std::string response = ollama_generate(cfg, prompt);

// Test: injected mock
auto mock_generate = [](const Config&, const std::string&) { return "mocked"; };
```

## Test style
Tests are written using doctest's built-in BDD syntax (`SCENARIO`/`GIVEN`/`WHEN`/`THEN`):

```cpp
SCENARIO("loading config") {
    GIVEN("OLLAMA_HOST is set") {
        setenv("OLLAMA_HOST", "10.0.0.1", 1);
        WHEN("config is loaded") {
            Config c = load_env();
            THEN("host is overridden") {
                CHECK(c.host == "10.0.0.1");
            }
        }
    }
}
```

### Why GWT over AAA?
- Reads as a specification — accessible for C++ newcomers
- Doctest supports it natively, no extra syntax needed
- Given/When/Then maps to Arrange/Act/Assert but with clearer intent

## Rationale
- doctest is the lightest C++ test framework — compiles fast, header-only, FetchContent compatible
- BDD-style `TEST_CASE`/`SUBCASE` maps naturally to describe/it patterns
- Function pointer injection is sufficient for mocking a single HTTP dependency
- No additional mock library keeps the dependency count low

## Consequences
- All existing `assert()` tests are migrated to doctest `CHECK()`/`REQUIRE()`
- `ollama_generate` signature may need to become injectable (function pointer or template)
- doctest is fetched via CMake FetchContent, same as cpp-httplib

### File naming convention
- `*_test.cpp` — unit test (one module in isolation)
- `*_it.cpp` — integration test (feature flow end-to-end)
- `test_helpers.h` — shared mocks and utilities
