---
summary: The proposed implementation for incremental refactoring to reduce technical debt in SonarCloud is accurate and detailed, providing a clear explanation of the phased approach to address shell script issues, high-complexity functions, and nested conditionals.
status: accepted
---

summary: The proposed implementation for incremental refactoring to reduce technical debt in SonarCloud is accurate and detailed, providing a clear explanation of the phased approach to address shell script issues, high-complexity functions, and nested conditionals.

# ADR-105: SonarCloud Technical Debt Reduction Strategy

## Context

SonarCloud reports 175 CRITICAL issues, primarily:

- **cpp:S134** (103) - Nested if/else depth > 3
- **cpp:S3776** (40) - Cyclomatic complexity > 10
- **shelldre:S131** (5) - Shell script issues

Without addressing these, code maintainability degrades and new issues accumulate.

## Decision

Implement incremental debt reduction:

1. **Phase 1** (this PR): Fix shell script issues (5 issues, ~30 min)
2. **Phase 2** (next PR): Refactor high-complexity functions (cpp:S3776, 40 issues)
3. **Phase 3** (follow-up): Flatten nested conditionals (cpp:S134, 103 issues)

## Implementation

- Use early returns to reduce nesting depth
- Extract helper functions to reduce complexity
- Add unit tests for refactored code
- Verify no behavior changes with existing tests

## Rationale

- **Incremental**: Avoid massive refactoring that breaks things
- **Measurable**: Each phase reduces specific rule violations
- **Safe**: Existing tests verify correctness
- **Sustainable**: Prevents future debt accumulation

## Consequences

- ✓ Improves code quality incrementally
- ✓ Reduces maintenance burden
- ✓ Enables better code reviews
- ⚠ Requires multiple PRs (not one-shot fix)
