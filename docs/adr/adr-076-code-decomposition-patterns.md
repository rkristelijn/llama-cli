# ADR-076: Code Decomposition Patterns

*Status*: Accepted · *Date*: 2026-05-05 · *Extends*: [ADR-065](adr-065-code-consistency-refactor.md), [ADR-061](adr-061-file-size-limits.md)

## Context

Complex conditionals and long functions reduce readability and testability. The codebase already follows "simple C++" (CONTRIBUTING.md), but lacks explicit guidance on *when* and *how* to decompose logic.

## Decision

Adopt three complementary decomposition patterns:

### 1. Extract Method (Fowler)

Replace inline logic with a named function that reveals intent.

```cpp
// ❌ Before — reader must parse the condition
if (person.age >= 18 && person.height_cm >= 120 && !person.has_medical_condition) {
  allow_ride();
}

// ✅ After — intent is immediately clear
if (can_ride(person)) {
  allow_ride();
}
```text

**When**: Any condition with 2+ clauses, or any expression that needs a comment to explain.

### 2. Single Level of Abstraction (SLA)

Each function operates at one abstraction level. Don't mix orchestration with detail.

```cpp
// ❌ Before — mixes high-level flow with low-level parsing
void process_response(const std::string& json) {
  size_t pos = json.find("\"content\":");
  // ... 20 lines of parsing ...
  render_markdown(content);
  log_event("response", content);
}

// ✅ After — each function is one level
void process_response(const std::string& json) {
  auto content = extract_content(json);
  render_markdown(content);
  log_event("response", content);
}
```text

**When**: A function has blocks that could be described with a comment header.

### 3. Composed Method (Beck)

A method should do one thing, and its body should be a sequence of steps at the same abstraction level — like a recipe.

```cpp
// ✅ Composed method — reads like a recipe
void handle_user_input(const std::string& input) {
  if (is_command(input))    return execute_command(input);
  if (is_file_ref(input))   return attach_file(input);
  send_to_llm(input);
}
```text

**When**: A function has early-return branches or a switch/if-else chain.

## Benefits

| Benefit | Mechanism |
|---------|-----------|
| **Readable** | Code reads as prose — no mental parsing needed |
| **Testable** | Each extracted function is independently testable |
| **Performant** | Compiler inlines small functions — zero runtime cost |
| **Cacheable** | Pure functions with clear inputs enable memoization |
| **Debuggable** | Stack traces show meaningful function names |

## Enforcement

1. **pmccabe** — cyclomatic complexity > 10 triggers a warning (existing)
2. **Semgrep rules** — detect complex inline conditions (`.config/patterns.yml`)
3. **`make learn`** — AI-assisted pattern discovery (see `scripts/dev/learn.sh`)
4. **PR review** — check against SLA principle

## Anti-patterns to avoid

| Anti-pattern | Why it's bad |
|--------------|-------------|
| Over-extraction | 1-line functions that add indirection without clarity |
| Naming by implementation | `checkAgeAndHeight()` instead of `canRide()` |
| Deep call chains | 5+ levels of delegation for simple logic |
| Premature abstraction | Extracting before you have 2+ callers |

## Consequences

- Functions stay short (< 20 lines preferred, < 40 max)
- Conditions read as domain language
- New developers understand flow without reading implementation
- `make complexity` catches violations automatically
