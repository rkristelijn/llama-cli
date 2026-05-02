# ADR-066: SOLID Refactoring Strategy

*Status*: Accepted  
*Date*: 2026-05-02  
*Supersedes*: Extends ADR-065 (code consistency refactor)

## Context

The codebase has grown organically. Tests exist and pass (154 scenarios), but several files violate SOLID principles, making them hard to test in isolation, hard to extend, and hard to maintain. ADR-065 identified the split targets; this ADR defines the *how* — principles, patterns, execution order, and enforcement.

### Current violations

| File | Lines | SOLID violation | Problem |
|------|-------|-----------------|---------|
| `repl.cpp` | 1998 | **SRP** — does input, commands, annotations, model selection, rendering | Cannot test one concern without loading all others |
| `tui.h` | 821 | **SRP/OCP** — inline monolith: colors, markdown, spinner, banner | Cannot extend rendering without modifying the header |
| `config.cpp` | 492 | **DIP** — `Config::instance()` singleton used alongside `const Config&` param | Dual access makes mocking impossible in some paths |
| `main.cpp` | 460 | **SRP** — loads config, kiro agents, dispatches sync, processes annotations | Hard to test dispatch logic independently |
| `help.h` | 97 | **OCP** — inline string literals grow with every new command | Every new feature touches this file |

### What SOLID means for this project

| Principle | Practical rule | Enforcement |
|-----------|---------------|-------------|
| **S**ingle Responsibility | One file = one reason to change. Max 300 lines per .cpp, 150 per .h | `check-file-size.sh` |
| **O**pen/Closed | New commands/renderers via new files, not editing existing ones | Code review + module pattern |
| **L**iskov Substitution | Injectable functions (ChatFn, ModelsFn) must be swappable without side effects | Unit tests with mocks |
| **I**nterface Segregation | Headers expose only what consumers need. No "include everything" headers | `check-consistency.sh` (unused includes) |
| **D**ependency Inversion | High-level modules depend on abstractions (function types), not concrete implementations | Constructor/function injection; no singleton access in business logic |

## Decision

### 1. Refactoring approach: Strangler Fig (one file at a time)

Each refactoring step:

1. Extract code to new file
2. Add `#include` in original, call extracted function
3. Run `make test` — must pass before proceeding
4. Commit

Never rewrite; always extract. This keeps the system stable at every commit.

### 2. Split plan (priority order)

#### Phase 1 — repl.cpp (SRP)

| New file | Responsibility | ~Lines |
|----------|---------------|--------|
| `src/repl/repl_commands.cpp` | Slash command dispatch (/set, /color, /model, /scan, /mem, /pref, /rate, /help, /clear, /version) | ~400 |
| `src/repl/repl_annotations.cpp` | Annotation handling (write, read, exec, str_replace) | ~350 |
| `src/repl/repl_model.cpp` | Model selection UI (handle_model_selection) | ~180 |
| `src/repl/repl_history.cpp` | Conversation history management, context building | ~150 |
| `src/repl/repl.cpp` | Core loop + dispatch (remains) | ~300 |

Each new file gets a matching `_test.cpp`. Headers expose only the function signatures needed by `repl.cpp`.

#### Phase 2 — tui.h (SRP + OCP)

| New file | Responsibility |
|----------|---------------|
| `src/tui/tui.h` | Declarations only (colors, types) |
| `src/tui/tui.cpp` | Color utilities, terminal detection |
| `src/tui/markdown.h` + `markdown.cpp` | StreamRenderer, markdown parsing |
| `src/tui/spinner.h` + `spinner.cpp` | Spinner animation |

#### Phase 3 — Config DIP

- Remove all `Config::instance()` calls from business logic
- Pass `const Config&` explicitly everywhere (already done in repl, ollama)
- `Config::instance()` only used in `main.cpp` for initial construction

#### Phase 4 — main.cpp (SRP)

| Extract to | Responsibility |
|-----------|---------------|
| `src/kiro/kiro.cpp` | `.kiro/agents` loading |
| `src/sync/sync_annotations.cpp` | `process_sync_annotations()` (already 110 lines) |

### 3. Dependency Injection pattern

Already established in `repl.h` — extend it:

```cpp
// Function type aliases (already exist)
using ChatFn = std::function<std::string(const std::vector<Message>&)>;
using StreamChatFn = std::function<std::string(const std::vector<Message>&, StreamCallback)>;

// New: for extracted modules
using CommandHandler = std::function<bool(const std::string& input, Config& cfg, std::ostream& out)>;
using AnnotationHandler = std::function<std::string(const std::string& response, const Config& cfg)>;
```

Rule: **any function that does I/O or network must be injectable via `std::function` parameter with a default that points to the real implementation.**

### 4. Enforcement scripts

#### Existing (keep)

- `scripts/lint/check-file-size.sh` — max lines per file
- `scripts/lint/check-complexity.sh` — cyclomatic complexity ≤ 10
- `scripts/lint/check-consistency.sh` — naming, include guards

#### New/Updated

| Script | Purpose | Autofix? |
|--------|---------|----------|
| Update `check-file-size.sh` | Lower threshold: 300 lines .cpp, 150 lines .h (warn at 80%) | No — manual split needed |
| `check-singleton.sh` | Flag `Config::instance()` outside `main.cpp` | No — manual refactor |
| `check-include-depth.sh` | Warn if a .h includes >5 other project headers | No |
| `scripts/fmt/format-code.sh` | clang-format (already exists) | **Yes** — autofix |

#### Pre-commit integration

The pre-commit hook (`scripts/git/pre-commit.sh`) already runs format + lint. Add the new checks to the lint phase.

### 5. Testability rules

- Every new `.cpp` file must have a `_test.cpp` with ≥1 test
- Extracted functions take `std::istream&`/`std::ostream&` for I/O (already the pattern in `run_repl`)
- No global state in extracted modules — receive everything via parameters
- Mock-friendly: use `std::function` types, not virtual classes (keeps it simple for a CLI app)

### 6. Execution order (1 PR per phase)

| PR | Scope | Risk | Verification |
|----|-------|------|-------------|
| 1 | Extract `repl_commands.cpp` | Low — pure extraction | `make test` + e2e pass |
| 2 | Extract `repl_annotations.cpp` | Low | `make test` + e2e pass |
| 3 | Extract `repl_model.cpp` + `repl_history.cpp` | Low | `make test` |
| 4 | Split `tui.h` → `.h` + `.cpp` + `markdown.cpp` + `spinner.cpp` | Medium — many includers | Full rebuild + all tests |
| 5 | Remove `Config::instance()` from non-main files | Medium | `check-singleton.sh` + tests |
| 6 | Extract kiro loader + sync annotations from `main.cpp` | Low | `make test` |
| 7 | Update `check-file-size.sh` thresholds | Low | CI green |

### 7. Iteration checklist (per extract step)

Single branch, single PR. Each iteration must leave the code stable:

```text
[ ] 1. Identify the block to extract (function/if-chain/handler)
[ ] 2. Create new .h with function signature
[ ] 3. Create new .cpp with the extracted code
[ ] 4. Replace original code with #include + call
[ ] 5. make quick  → must pass (build + unit tests + comment ratio)
[ ] 6. make test   → must pass (unit + e2e)
[ ] 7. Commit: refactor(repl): extract <thing> to <file> [SRP]
[ ] 8. Repeat next block
```

If `make quick` fails → fix before moving on. Never stack broken extracts.

### 8. SOLID comment convention

When applying a SOLID principle, add a brief comment explaining *which* principle and *why*:

```cpp
// SRP: slash commands extracted — repl.cpp only handles the input loop now.
#include "repl/repl_commands.h"

// DIP: injectable chat function so tests can substitute a mock without network.
using ChatFn = std::function<std::string(const std::vector<Message>&)>;

// OCP: new commands are added as new handler functions, not by editing dispatch_command().
bool handle_set(const std::string& args, Config& cfg, std::ostream& out);

// ISP: repl_commands.h only exposes dispatch_command(), not internal helpers.
bool dispatch_command(const std::string& input, Config& cfg, std::ostream& out);

// LSP: any ChatFn (real or mock) must return a valid response string — no side effects.
```

Format: `// <PRINCIPLE>: <one-line reason>`

Only add these at the *boundary* where the principle is applied (file header, type alias, function signature). Not on every line.

## Consequences

- All source files stay under 300 lines (enforced)
- Each module is independently testable via injection
- New features (commands, renderers) are added as new files, not edits to existing ones
- CI catches regressions at every step (strangler fig approach)
- Slightly more files in `src/repl/` and `src/tui/` — acceptable trade-off for clarity

## References

- ADR-065 — Code consistency refactor (item list)
- ADR-018 — Module layout conventions
- ADR-008 — Test framework and naming
- ADR-055 — Layered test strategy
- Martin, R.C. — *Clean Architecture* (SOLID principles)

## Appendix A: Battle-Testing llama-cli as Async Worker

*Date*: 2026-05-02

### Experiment

Delegated a small refactoring task to llama-cli in sync mode: fix a broken path in `scripts/dev/quick.sh` (replace `scripts/check/comment-ratio.sh` with `scripts/lint/check-comment-ratio.sh`).

### Prompt format tested

Following the structured format from `docs/prompts/`:

```text
--system-prompt="You are a bash developer. You fix bugs in shell scripts.
                 Reply with ONLY the corrected code, no explanation."

Prompt (positional arg):
## Context
File: scripts/dev/quick.sh
Line 22 references scripts/check/comment-ratio.sh but that path no longer exists.
The correct path is scripts/lint/check-comment-ratio.sh

## Task
Fix line 22 by replacing the old path with the new path.

## Input (broken)
bash scripts/check/comment-ratio.sh | grep "PASS" || bash scripts/check/comment-ratio.sh

## Expected output
The same line with both paths replaced by scripts/lint/check-comment-ratio.sh
```

### Results

| Model | Result | Issue |
|-------|--------|-------|
| gemma4:26b | ❌ Hallucinated `echo "gemma4:26b"` or Python code | Thinking model: `<think>` tokens corrupt sync mode output |
| gemma4:26b (via `ollama run`) | ✅ Correct answer | Works fine outside llama-cli |
| mistral-nemo:12b | ⏱️ Slow to respond, not tested to completion | — |

### Root cause

**Twee bugs gevonden:**

1. **`--model VALUE` (spatie) werd niet geparsed** — alleen `--model=VALUE` en `-m VALUE` werkten. Het model-naam argument werd als positional arg (= prompt) geïnterpreteerd. Fix: `match_string_opts` en `match_int_opts` ondersteunen nu ook `--flag VALUE` naast `--flag=VALUE`.

2. **Sync mode dubbel-printte responses** — streaming callback schreef tokens naar stdout, daarna werd de volledige response nogmaals geprint. Fix: na streaming alleen een newline toevoegen.

**Niet de oorzaak:** thinking tokens. Ollama stuurt die in een apart `"thinking"` JSON veld — de `"content"` is leeg tijdens het denken. Geen filtering nodig.

### Findings

| What works | What doesn't |
|-----------|-------------|
| System prompt = identity/role ("You are a bash developer") | Putting the full task in system prompt |
| Prompt = structured task (Context → Task → Input → Expected) | Vague prompts ("fix this") |
| `ollama run` direct invocation | llama-cli sync mode with thinking models |
| Simple find-and-replace tasks | Tasks requiring multi-step reasoning |

### Recommendations

1. **Altijd `--model=` of `-m` gebruiken** — nu ook `--model VALUE` (spatie) ondersteund
2. **Structured prompt format** — altijd Context/Task/Input/Expected structure uit `docs/prompts/`
3. **System prompt = role only** — kort: wie je bent, hoe je antwoordt
4. **Prompt = the work** — alle context, taak, en verwacht output format in het positional argument
5. **gemma4:26b werkt prima** — thinking models zijn geen probleem, Ollama handelt het af

### Prompt template for delegation

```bash
./build/llama-cli --model <non-thinking-model> \
  --system-prompt="You are a <role>. You <constraint>. Reply with ONLY <format>." \
  "## Context
<what exists, what's relevant>

## Task
<exactly what to do>

## Input
<current state / broken code>

## Expected output
<what the answer should look like>"
```
