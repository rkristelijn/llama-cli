---
status: accepted
---

# ADR-104: i18n String Centralization

## Context

User-facing strings (error messages, prompts, UI text) are scattered throughout the codebase as hardcoded literals. This makes:

- Localization impossible
- Maintenance harder (strings duplicated across files)
- Testing more fragile (string changes break tests)

## Decision

Centralize all user-facing strings in `src/ui/messages.h` as constants. This provides:

1. Single source of truth for UI text
2. Foundation for future i18n/localization
3. Easier testing (mock messages)
4. Consistent terminology across UI

## Implementation

- `src/ui/messages.h` — Message constants (e.g., `MSG_ERROR_FILE_NOT_FOUND`)
- `scripts/lint/check-user-facing-strings.sh` — Detect remaining hardcoded strings
- Gradual migration: refactor high-impact areas first (error messages, prompts)

## Rationale

Centralization is low-risk when done incrementally:

- Start with error messages (highest impact)
- Add regression tests to prevent backsliding
- Refactor one module at a time
- Keep detection script to catch new violations

## Consequences

- ✅ Enables future localization
- ✅ Improves code maintainability
- ✅ Reduces string duplication
- ⚠️ Requires gradual refactoring (not all at once)
