# Prompt 04: Add CHANGELOG.md

## Context

- The project uses semantic versioning (VERSION file at project root)
- Releases are published via GitHub Actions (`.github/workflows/release.yml`)
- There is no CHANGELOG.md yet
- Format: [Keep a Changelog](https://keepachangelog.com/)

## Task

Create `CHANGELOG.md` at the project root.

## File: Create `CHANGELOG.md`

```markdown
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- Quality framework (ADR-048) with CMMI maturity levels
- AI agent task prompts in `docs/prompts/`
- GitHub issue and PR templates
- `.env.example` configuration template
- Commit message validation hook
- Branch naming validation

## [1.0.0] - 2026-04-18

### Added

- Interactive REPL with conversation memory
- File writing from LLM responses (`<write>`)
- Targeted edits (`<str_replace>`)
- Smart file reading (`<read>` with line ranges and search)
- Command execution (`!`, `!!`, `<exec>`)
- TUI with ANSI colors, markdown rendering, spinner
- Arrow key history (linenoise)
- Runtime options (`/set markdown`, `/set color`, `/set bofh`)
- Ctrl+C interrupt during LLM calls
- Stdin pipe support (`--files`)
- Auto-diff preview before file writes
- Configurable host, port, model, timeout
- CI pipeline with 16 quality checks
- Pre-commit hooks (format, lint, secrets)
- E2E test suite (5 scenarios)
```

## Verify

```bash
# 1. File exists
test -f CHANGELOG.md && echo "PASS: CHANGELOG.md exists" || echo "FAIL: file not found"

# 2. Has required sections
grep -c "Unreleased" CHANGELOG.md && echo "PASS: has Unreleased section"
grep -c "Keep a Changelog" CHANGELOG.md && echo "PASS: has format reference"
grep -c "Semantic Versioning" CHANGELOG.md && echo "PASS: has semver reference"
```

## Expected output

```text
PASS: CHANGELOG.md exists
PASS: has Unreleased section
PASS: has format reference
PASS: has semver reference
```

## Commit message

```text
chore: add CHANGELOG.md
```
