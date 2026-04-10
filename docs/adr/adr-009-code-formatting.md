# ADR-009: Code Formatting

*Status*: Accepted · *Date*: 2026-04-10 · *Context*: Inconsistent indentation (2 vs 4 spaces) across the codebase. A single formatting standard is needed that is enforced automatically.

## Decision
`.editorconfig` and `.clang-format` are used together. Formatting is auto-fixed in the pre-commit hook.

| Tool | Role |
|------|------|
| `.editorconfig` | Editor-level defaults (indent, charset, line endings) |
| `.clang-format` | C++ formatting enforcement (exact style) |
| Pre-commit hook | Auto-runs `clang-format` on staged files before commit |

### Style choices
- 2-space indent
- No tabs
- 120 char line width
- Based on LLVM style (closest to the desired look)

### Workflow
1. Developer writes code (any style)
2. `git commit` triggers pre-commit hook
3. Hook runs `clang-format -i` on staged `.cpp` and `.h` files
4. Files are re-staged with correct formatting
5. Commit proceeds with clean code

No manual formatting needed. No style debates. The tool decides.

## Rationale
- `.editorconfig` is editor-agnostic — works in VS Code, Vim, CLion, etc.
- `.clang-format` is the C++ standard for formatting — used by LLVM, Google, Chromium
- Auto-fix in pre-commit means formatting is never forgotten
- Developers don't need to think about style — write code, commit, done

## Consequences
- `clang-format` must be installed locally (`brew install clang-format` / `apt install clang-format`)
- All existing code is reformatted in one commit
- CI could optionally check formatting (future)
