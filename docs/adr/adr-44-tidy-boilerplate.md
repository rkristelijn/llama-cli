# ADR-044: Tidy Build Boilerplate

*Status*: Proposed · *Date*: 2026-04-17

## Context

The project's build automation has grown organically across three layers — Makefile (~300 lines), 13 shell scripts, and a CI workflow with 15 jobs. Each layer contains inline logic that overlaps with the others, naming conventions are inconsistent, and version pinning is incomplete. This ADR addresses six concerns:

1. **Makefile responsibility** — targets contain 10–30 line shell blocks (tidy, lint, complexity, coverage) that are hard to test and debug
2. **Script naming** — mixed conventions: `gh-create-pr.sh` (kebab), `test_comment_ratio.sh` (snake), `build-index.sh` (kebab, no context prefix)
3. **Script organization** — flat `scripts/` directory with 13 files, no grouping by purpose
4. **CI workflow** — each job installs its own dependencies inline; logic is not reproducible locally
5. **Version pinning** — `.config/versions.json` pins only 4 of ~12 tools; `setup.sh` installs most tools unpinned
6. **Self-documenting build** — `make help` is manually maintained and incomplete; no progress feedback during `make check`; no self-recovery when tools are missing

### Current state

| Area | Now | Problem |
|------|-----|---------|
| Makefile | ~300 lines, inline shell blocks for tidy/lint/complexity/coverage | Can't test or debug recipe logic independently |
| Scripts | 13 files, flat, mixed naming (`gh-*`, `test_*`, `build-*`) | No convention, hard to discover purpose |
| CI | 15 jobs, each with inline `apt-get install` + tool logic | Not reproducible locally, duplicated install steps |
| Version pinning | `versions.json` pins 4 tools (doxygen, clang-format, clang-tidy, git_cliff) | cmake, cppcheck, pmccabe, semgrep, gitleaks, cloc unpinned |
| `setup.sh` | brew/apt detection, no version pinning for most tools | Different versions across machines |
| `make help` | Manual echo block, 13 of ~30 targets listed | Gets out of sync, missing targets |
| Progress output | `==> make X` / `[done]` pattern | No total count, no summary on partial failure |
| Missing tools | `check-deps` only checks cmake | Other tools fail with cryptic errors |
| Missing tools: markup linting, dead code finder, duplication finder, unsure if there are tools out there that would fit our project |  |  |

## Decisions

### 1. Makefile responsibility boundary

**Decision: Hybrid approach — keep compilation inline, extract procedural logic to scripts.**

| Option | Description | Advice |
|--------|-------------|--------|
| **A. Pure dispatcher** (Kubernetes-style) | Every target is a 1-line script call | ✗ Loses Make's dependency tracking, which is genuinely useful for C compilation |
| **B. Monolithic** (current / Redis-style) | All logic inline in Makefile | ✗ 10–30 line shell blocks are untestable and hard to debug |
| **C. Hybrid** (Git-style) | Compilation and simple targets inline; procedural logic (>5 lines, conditionals, loops) in scripts | **✓ Recommended** — uses Make for what it's good at, scripts for what they're good at |

**The rule**: if a recipe has conditionals, loops, or exceeds ~5 lines of shell, extract it to a script. Compilation rules, `clean`, `install`, and simple phony targets stay inline.

**Targets to extract** (current inline logic → script):

| Target | Current lines | Extract to |
|--------|--------------|------------|
| `tidy` | 18 lines, if/else, for loop | `scripts/check/tidy.sh` |
| `complexity` | 8 lines, while loop, awk | `scripts/check/complexity.sh` |
| `coverage-folder` | 12 lines, for loop, awk | `scripts/check/coverage-folder.sh` |
| `todo` | 12 lines, awk, grep | `scripts/dev/todo.sh` |
| `e2e` | 5 lines, for loop, case | `scripts/check/e2e.sh` |
| `comment-ratio` | 4 lines, awk pipeline | keep inline (no conditionals, short) |
| `lint` | 3 lines | keep inline |
| `format` / `format-check` | 2 lines each | keep inline |

After extraction, the Makefile target becomes:

```makefile
tidy: all
	@bash scripts/check/tidy.sh $(if $(filter 1,$(FULL)),--full)
```

### 2. Script naming convention

**Decision: kebab-case, verb-object pattern.**

| Option | Description | Advice |
|--------|-------------|--------|
| **A. Flat with context prefix** (`ci-build.sh`, `gh-create-pr.sh`) | All scripts in `scripts/`, prefixed by context | ✗ Doesn't scale past ~15 scripts; tab-completion is noisy |
| **B. Subdirectories with verb-object names** | `scripts/ci/build.sh`, `scripts/gh/create-pr.sh` | **✓ Recommended** — scales, discoverable, matches Kubernetes/Git patterns |
| **C. Subdirectories with context-verb-object names** | `scripts/ci/ci-build.sh` | ✗ Redundant — the directory already provides context |

**Naming rules**:

- **kebab-case** for filenames (POSIX/Linux convention, used by Kubernetes, Git, Docker)
- **verb-object** pattern: `check-format.sh`, `create-pr.sh`, `run-coverage.sh`
- **`.sh` extension** on all scripts (aids syntax highlighting, makes language obvious)
- **snake_case** for variables and functions inside scripts

**Proposed directory structure**:

```text
scripts/
├── check/                  # Quality gate scripts (called by make check)
│   ├── tidy.sh
│   ├── complexity.sh
│   ├── coverage-folder.sh
│   ├── comment-ratio.sh
│   └── e2e.sh
├── ci/                     # CI-specific scripts
│   └── install-deps.sh     # Shared dependency installation
├── dev/                    # Local development helpers
│   ├── setup.sh
│   ├── log-viewer.sh
│   ├── todo.sh
│   └── build-index.sh
├── gh/                     # GitHub CLI helpers
│   ├── create-pr.sh
│   ├── create-issue.sh
│   ├── download-issues.sh
│   ├── pipeline-status.sh
│   ├── pr-status.sh
│   └── pr-feedback.sh
└── test/                   # Test infrastructure
    ├── test-index.sh
    └── test-files-integration.sh
```

**Migration from current names**:

| Current | New |
|---------|-----|
| `scripts/gh-create-pr.sh` | `scripts/gh/create-pr.sh` |
| `scripts/gh-create-issue.sh` | `scripts/gh/create-issue.sh` |
| `scripts/gh-download-issues.sh` | `scripts/gh/download-issues.sh` |
| `scripts/gh-pipeline-status.sh` | `scripts/gh/pipeline-status.sh` |
| `scripts/gh-pr-status.sh` | `scripts/gh/pr-status.sh` |
| `scripts/gh-pr-feedback.sh` | `scripts/gh/pr-feedback.sh` |
| `scripts/test_comment_ratio.sh` | `scripts/check/comment-ratio.sh` |
| `scripts/test_coverage.sh` | `scripts/check/coverage-folder.sh` |
| `scripts/test-index.sh` | `scripts/test/test-index.sh` |
| `scripts/test-files-integration.sh` | `scripts/test/test-files-integration.sh` |
| `scripts/build-index.sh` | `scripts/dev/build-index.sh` |
| `scripts/log-viewer.sh` | `scripts/dev/log-viewer.sh` |
| `scripts/setup.sh` | `scripts/dev/setup.sh` |

### 3. CI as thin proxy

**Decision: CI jobs call `make` targets or scripts; install logic moves to reusable scripts.**

| Option | Description | Advice |
|--------|-------------|--------|
| **A. Inline everything** (current) | Each job has its own `apt-get install` + tool logic | ✗ Not reproducible locally, duplicated install steps |
| **B. Composite actions** | Shared setup steps as `.github/actions/setup-*/action.yml` | ○ Good for multi-repo; overkill for single repo |
| **C. Thin proxy to make/scripts** | CI jobs just call `make <target>`, scripts handle deps | **✓ Recommended** — reproducible locally, CI-vendor portable |

**What stays in YAML** (CI-specific concerns only):

- Trigger configuration, runner/OS selection, matrix strategy
- `actions/checkout`, `actions/cache`, `actions/upload-artifact` (GitHub-specific APIs)
- Job dependency graph and parallelism
- Secrets injection

**What moves to scripts/make**:

- Tool installation → `scripts/ci/install-deps.sh` (reads `versions.env`, installs what's missing)
- All build/test/lint/check logic → `make <target>` (already mostly true)

**Additional CI improvements**:

- Pin `ubuntu-24.04` instead of `ubuntu-latest` (known CI anti-pattern; breaks without warning during migrations)
- Pin third-party actions to commit SHA with version comment (the March 2025 `tj-actions/changed-files` supply chain attack affected 23,000 repos via mutable tags)
- Add `concurrency` groups to cancel superseded runs on the same branch

### 4. Version pinning

**Decision: `versions.env` as single source of truth, read by `setup.sh` and CI.**

ADR-026 established the principle; this extends it to all tools.

| Option | Description | Advice |
|--------|-------------|--------|
| **A. `versions.json`** (current) | JSON file, read with `jq` | ○ Works but requires `jq` as a bootstrap dependency |
| **B. `versions.env`** | Shell-sourceable `KEY=VALUE` file | **✓ Recommended** — zero dependencies to parse, works in Make and shell |
| **C. `mise.toml` / `.tool-versions`** | Declarative tool manager | ○ Good if team uses mise; adds external dependency |
| **D. Nix flakes** | Hermetic, reproducible environments | ✗ Steep learning curve, overkill for this project |

**Proposed `versions.env`**:

```bash
# .config/versions.env — single source of truth for tool versions
# Read by: Makefile, scripts/dev/setup.sh, scripts/ci/install-deps.sh
CMAKE_VERSION=3.28
CLANG_FORMAT_VERSION=19
CLANG_TIDY_VERSION=19
CPPCHECK_VERSION=2.13
PMCCABE_VERSION=2.8
DOXYGEN_VERSION=1.16.1
SEMGREP_VERSION=1.56.0
GITLEAKS_VERSION=8.18.2
GIT_CLIFF_VERSION=2.12.0
CLOC_VERSION=2.02
```

**Install fallback priority** (in `setup.sh`):

1. Check if correct version already installed → skip
2. Platform package manager (brew/apt) with version pin
3. Direct download from GitHub releases / official site (for tools not in apt, e.g., gitleaks, git-cliff)
4. Fail with clear error message and install instructions

**Validation**: add a `make check-versions` target that compares installed versions against `versions.env` and warns on mismatches.

### 5. Self-documenting build

**Decision: `##` comment extraction for help, numbered progress, guard macros for missing tools.**

#### 5a. `make help`

| Option | Description | Advice |
|--------|-------------|--------|
| **A. Manual echo** (current) | Hand-written help block | ✗ Gets out of sync (currently missing ~17 targets) |
| **B. `##` comment extraction** | `grep`/`awk` extracts `target: ## description` | **✓ Recommended** — single source of truth, stays in sync |
| **C. Hybrid** | Manual curated "quick start" + auto-generated full list | ○ Best of both worlds, but more complex |

Recommended: Option B with `##@` section headers for grouping. Add `.DEFAULT_GOAL := help` so bare `make` shows help.

```makefile
.DEFAULT_GOAL := help

##@ Getting Started
setup: ## Install all dependencies
build: ## Build the project
start: ## Build and run the REPL (alias: s)

##@ Quality
check: ## Run smart quality checks (alias: c)
full-check: ## Run exhaustive quality checks (alias: fc)
test:  ## Run unit tests (alias: t)

##@ GitHub
gh-pr-status: ## Show failed PR jobs (alias: gps)

help: ## Show this help
	@awk 'BEGIN {FS = ":.*##"; printf "Usage:\n  make \033[36m<target>\033[0m\n"} \
		/^[a-zA-Z_0-9-]+:.*?##/ { printf "  \033[36m%-25s\033[0m %s\n", $$1, $$2 } \
		/^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) }' $(MAKEFILE_LIST)
```

#### 5b. Progress output

Replace the current `==> make X` / `[done]` pattern with numbered steps:

```text
[1/9] format-check... ✓
[2/9] tidy............. ✓
[3/9] complexity....... ✓
[4/9] lint............. ✓
[5/9] docs............. ✓
[6/9] coverage......... ✓
[7/9] sast-security.... ✓
[8/9] sast-secret...... ✓
[9/9] comment-ratio.... ✓

All 9 checks passed.
```

Quiet by default, verbose on failure (suppress tool output during success, dump it on error).

#### 5c. Missing tool recovery

Strengthen `check-deps` to cover all required tools, with specific install instructions:

```makefile
check-deps:
	@missing=0; \
	for tool in cmake cppcheck pmccabe cloc; do \
		command -v $$tool >/dev/null 2>&1 || { echo "ERROR: $$tool not found"; missing=1; }; \
	done; \
	[ $$missing -eq 0 ] || { echo "Run 'make setup' to install missing tools."; exit 1; }
```

Distinguish **required** tools (cmake — hard fail) from **optional** tools (semgrep, gitleaks — soft skip with message). The current pattern for sast targets already does this correctly.

### 6. Tool documentation

**Decision: Add `docs/tools/<toolname>.md` for each pinned tool.**

Each file documents: version, purpose, usage examples, troubleshooting, and what works/doesn't work in this project. This serves as a reference for contributors and AI assistants.

```text
docs/tools/
├── clang-format.md
├── clang-tidy.md
├── cppcheck.md
├── doxygen.md
├── gitleaks.md
├── pmccabe.md
└── semgrep.md
```

note: there might be tools description already, but in docs/* like clang-tidy.md code-rabbit.md, gh-manual.md github-integration.md, ollama-setup.md

### 7. Naming convention enforcement

**Decision: Add a lint script that validates script naming and Makefile structure.**

A `scripts/check/lint-conventions.sh` script that verifies:

- All scripts in `scripts/` use kebab-case
- All scripts have a header comment in the first 5 lines
- All Makefile targets with >5 lines of shell delegate to a script
- CI workflow jobs call `make` or `scripts/` (no inline logic beyond setup)

This is low-effort (~30 lines) and catches drift. Add to `make check`.

## Acceptance Criteria

- [ ] Makefile targets with >5 lines of shell are extracted to scripts
- [ ] All scripts use kebab-case, verb-object naming
- [ ] Scripts are organized in subdirectories: `check/`, `ci/`, `dev/`, `gh/`, `test/`
- [ ] CI jobs call `make`/scripts instead of inline logic
- [ ] CI pins `ubuntu-24.04` and action SHAs
- [ ] `versions.env` pins all tools; `setup.sh` reads it
- [ ] `setup.sh` works on both macOS (brew) and Linux (apt) with version pinning
- [ ] `make help` is auto-generated from `##` comments with `##@` section headers
- [ ] `.DEFAULT_GOAL := help`
- [ ] `make check` shows numbered progress `[n/total]`
- [ ] `check-deps` validates all required tools with install instructions
- [ ] Optional tools (semgrep, gitleaks) soft-skip with message when missing
- [ ] `docs/tools/<toolname>.md` exists for each pinned tool
- [ ] Convention lint script validates naming and structure
- [ ] `make coverage` target works standalone (currently only `coverage-folder` exists as a check target)
- [ ] All existing `make check` sub-targets still pass after migration

## Consequences

- Makefile shrinks from ~300 lines to ~150 lines (orchestration only)
- Scripts become independently testable and debuggable (`bash -x scripts/check/tidy.sh`)
- CI becomes reproducible locally (`make check` runs the same checks as CI)
- New contributors type `make` → see help → know what to do
- Version drift across machines is caught by `make check-versions`
- Migration is incremental — extract one target at a time, verify `make check` still passes

## References

- @see docs/adr/adr-023-self-documenting-processes.md — self-documentation principles
- @see docs/adr/adr-026-version-pinning.md — version pinning rationale (this ADR extends it)
- @see docs/adr/adr-002-quality-checks.md — quality gate definitions
- @see docs/adr/adr-011-self-contained-setup.md — `make setup` portability
- Kubernetes Makefile pattern: github.com/kubernetes/kubernetes/blob/master/build/root/Makefile
- Git CI scripts: github.com/git/git/tree/master/ci
- Self-documenting Makefiles: marmelab.com/blog/2016/02/29/auto-documented-makefile.html
- Redbubble "Makefile to Bash and back": medium.com/redbubble (2018)
- Shell conventions: docs/tools/shell-scripts.md
- Google Shell Style Guide: google.github.io/styleguide/shellguide.html
- Shell Script Best Practices: sharats.me/posts/shell-script-best-practices/
- ShellCheck: shellcheck.net

---

## Implementation Plan

Each phase produces a stable, passing codebase. Ordered: make it work → make it better →
validate assumptions.

> **Rules**: CONTRIBUTING.md · **Shell conventions**: docs/tools/shell-scripts.md

### Acceptance criteria (grouped by phase)

#### Group A — Foundation (Phase 1–2)

- [ ] `versions.env` pins all tools; `setup.sh` reads it
- [ ] `setup.sh` works on both macOS (brew) and Linux (apt) with version pinning
- [ ] `.shellcheckrc` exists in project root
- [ ] ShellCheck passes on all existing scripts
- [ ] `make shellcheck` target exists

#### Group B — Script migration (Phase 3–4)

- [ ] All scripts use `#!/usr/bin/env bash` and `set -o errexit/nounset/pipefail`
- [ ] All scripts use kebab-case, verb-object naming
- [ ] Scripts organized in subdirectories: `check/`, `ci/`, `dev/`, `gh/`, `test/`
- [ ] Makefile targets with >5 lines of shell extracted to scripts
- [ ] `make check` still passes after each migration step

#### Group C — Self-documenting build (Phase 5)

- [ ] `make help` auto-generated from `##` comments with `##@` section headers
- [ ] `.DEFAULT_GOAL := help`
- [ ] `make check` shows numbered progress `[n/total]`
- [ ] `check-deps` validates all required tools with install instructions
- [ ] Optional tools soft-skip with message when missing
- [ ] `make coverage` works standalone

#### Group D — CI and enforcement (Phase 6–7)

- [ ] CI jobs call `make`/scripts instead of inline logic
- [ ] CI pins `ubuntu-24.04` and action SHAs
- [ ] Convention lint script validates naming and structure
- [ ] `docs/tools/<toolname>.md` exists for each pinned tool

---

### Phase 1 — Version pinning foundation

**Goal**: Single source of truth for tool versions. `make setup` respects it.

**Why first**: Everything else depends on consistent tool versions. Lowest-risk change —
adds a file and updates `setup.sh` to read it.

**Steps**:

1. Create `versions.env` in project root:

   ```bash
   # versions.env — single source of truth for tool versions
   # Read by: Makefile, scripts/dev/setup.sh, scripts/ci/install-deps.sh
   # @see docs/adr/adr-026-version-pinning.md
   CMAKE_VERSION=3.28
   CLANG_FORMAT_VERSION=19
   CLANG_TIDY_VERSION=19
   CPPCHECK_VERSION=2.13
   PMCCABE_VERSION=2.8
   DOXYGEN_VERSION=1.16.1
   SEMGREP_VERSION=1.56.0
   GITLEAKS_VERSION=8.18.2
   GIT_CLIFF_VERSION=2.12.0
   CLOC_VERSION=2.02
   SHELLCHECK_VERSION=0.10.0
   ```

2. Update `setup.sh` to source `versions.env` instead of `jq` from `versions.json`.
   Add version checking: compare installed vs pinned. Add ShellCheck to install list.

3. Add `make check-versions` target that warns on mismatches.

4. Update CI workflow to source `versions.env`. Remove `.config/versions.json`.

**Validate**: `make setup && make check-versions && make check`

**Branch**: `adr-44/version-pinning`

---

### Phase 2 — ShellCheck and script hygiene

**Goal**: All existing scripts pass ShellCheck. Establish linting baseline.

**Why second**: Before reorganizing scripts, make them correct. ShellCheck catches real
bugs that would be painful to debug during migration.

**Steps**:

1. Create `.shellcheckrc` in project root:

   ```text
   shell=bash
   ```

2. Run ShellCheck on all scripts, fix issues:
   - `#!/bin/sh` → `#!/usr/bin/env bash`
   - Add `set -o errexit`, `set -o nounset`, `set -o pipefail`
   - Add TRACE support: `if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi`
   - Quote unquoted variables
   - `[ ]` → `[[ ]]`
   - Add file header comments

3. Add `make shellcheck` target:

   ```makefile
   shellcheck: ## Lint shell scripts
   	@find scripts e2e -name '*.sh' | xargs shellcheck
   ```

4. Add `shellcheck` to `make check` pipeline.

**Validate**: `make shellcheck && make check`

**Branch**: `adr-44/shellcheck`

---

### Phase 3 — Script reorganization

**Goal**: Move scripts to subdirectories with consistent naming.

**Why third**: Scripts are now correct (Phase 2). This is a pure rename/move — no logic
changes.

**Steps**:

1. Create subdirectories: `mkdir -p scripts/{check,ci,dev,gh,test}`

2. Move and rename (one at a time, verify after each):

   | Current | New |
   |---------|-----|
   | `scripts/test_comment_ratio.sh` | `scripts/check/comment-ratio.sh` |
   | `scripts/test_coverage.sh` | `scripts/check/coverage.sh` |
   | `scripts/setup.sh` | `scripts/dev/setup.sh` |
   | `scripts/build-index.sh` | `scripts/dev/build-index.sh` |
   | `scripts/log-viewer.sh` | `scripts/dev/log-viewer.sh` |
   | `scripts/gh-create-pr.sh` | `scripts/gh/create-pr.sh` |
   | `scripts/gh-create-issue.sh` | `scripts/gh/create-issue.sh` |
   | `scripts/gh-download-issues.sh` | `scripts/gh/download-issues.sh` |
   | `scripts/gh-pipeline-status.sh` | `scripts/gh/pipeline-status.sh` |
   | `scripts/gh-pr-status.sh` | `scripts/gh/pr-status.sh` |
   | `scripts/gh-pr-feedback.sh` | `scripts/gh/pr-feedback.sh` |
   | `scripts/test-index.sh` | `scripts/test/test-index.sh` |
   | `scripts/test-files-integration.sh` | `scripts/test/test-files-integration.sh` |

3. Update all Makefile references to new paths.

4. Update cross-references in docs, CI workflow, and other scripts.

**Validate**: `make check && make shellcheck && make gh-pr-status`

**Branch**: `adr-44/script-reorg`

---

### Phase 4 — Extract Makefile logic to scripts

**Goal**: Makefile targets with >5 lines of shell delegate to scripts.

**Why fourth**: Scripts are organized (Phase 3). Each extraction is independent — do one,
verify, commit, repeat.

**Extraction order** (simplest first):

| # | Target | Lines | Extract to | Notes |
|---|--------|-------|------------|-------|
| 1 | `e2e` | 5 | `scripts/check/e2e.sh` | Simple loop |
| 2 | `complexity` | 8 | `scripts/check/complexity.sh` | while + awk |
| 3 | `todo` | 12 | `scripts/dev/todo.sh` | awk + grep |
| 4 | `coverage-folder` | 12 | `scripts/check/coverage-folder.sh` | for + awk |
| 5 | `tidy` | 18 | `scripts/check/tidy.sh` | if/else + for |

**Targets that stay inline** (no extraction needed):

| Target | Lines | Why |
|--------|-------|-----|
| `all` / `build` | 3 | Pure Make — cmake dependency tracking |
| `clean` | 1 | Trivial |
| `install` | 2 | Trivial |
| `format` / `format-check` | 2 each | Simple find + xargs |
| `lint` | 3 | Single command with flags |
| `comment-ratio` | 4 | Single pipeline |
| `sast-*` | 4 each | Simple if/command check |

**Example — extracting `e2e`**:

Makefile after:

```makefile
e2e: build ## Run end-to-end tests
	@bash scripts/check/e2e.sh "$(BUILD_DIR)/llama-cli"
```

New script follows template from docs/tools/shell-scripts.md: shebang, safety flags,
TRACE, file header, `main()` pattern, ~20% comments.

**Validate after each extraction**: `make <target> && make check && make shellcheck`

**Branch**: `adr-44/extract-logic`

---

### Phase 5 — Self-documenting build

**Goal**: Auto-generated help, numbered progress, comprehensive tool detection.

**Steps**:

#### 5a. Auto-generated `make help`

Add `##` comments to every target and `##@` section headers:

```makefile
.DEFAULT_GOAL := help

##@ Getting Started
setup: ## Install all dependencies
build: ## Build the project
start: ## Build and run the REPL (alias: s)

##@ Quality
check: ## Run smart quality checks (alias: c)
full-check: ## Run exhaustive quality checks (alias: fc)
test: ## Run unit tests (alias: t)

##@ GitHub
gh-pr-status: ## Show failed PR jobs (alias: gps)
```

Replace manual help with awk extraction:

```makefile
help: ## Show this help
	@awk 'BEGIN {FS = ":.*##"; printf "Usage:\n  make \033[36m<target>\033[0m\n"} \
		/^[a-zA-Z_0-9-]+:.*?##/ { printf "  \033[36m%-25s\033[0m %s\n", $$1, $$2 } \
		/^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) }' $(MAKEFILE_LIST)
```

#### 5b. Numbered progress in `make check`

Create `scripts/check/run-all.sh`:

```text
[1/9] format-check... ✓
[2/9] tidy............ ✓
...
All 9 checks passed.
```

Quiet by default, verbose on failure (suppress output during success, dump on error).

#### 5c. Comprehensive `check-deps`

Create `scripts/check/check-deps.sh` that distinguishes:

- **Required** (cmake, cppcheck, pmccabe, cloc, clang-format, clang-tidy, doxygen) → hard fail
- **Optional** (semgrep, gitleaks, shellcheck) → warn, soft skip

**Validate**: `make && make help && make check && make check-deps`

**Branch**: `adr-44/self-doc`

---

### Phase 6 — CI as thin proxy

**Goal**: CI calls `make`/scripts. No inline build logic.

**Steps**:

1. Pin `ubuntu-24.04` (replace all `ubuntu-latest`).

2. Pin action SHAs with version comments:

   ```yaml
   - uses: actions/checkout@<sha> # v4.2.2
   ```

3. Create `scripts/ci/install-deps.sh` — reads `versions.env`, installs tools.
   Replaces inline `apt-get install` + LLVM repo setup in each job.

4. Simplify CI jobs to:

   ```yaml
   - name: Install dependencies
     run: bash scripts/ci/install-deps.sh clang-format clang-tidy
   - name: Quality checks
     run: make format-check
   ```

5. Add `concurrency` groups to cancel superseded runs.

6. Consider consolidating jobs — group format + tidy + lint + complexity into one
   "quality" job running `make check` to reduce runner overhead.

**Validate**: push branch, verify all CI jobs pass.

**Branch**: `adr-44/ci-proxy`

---

### Phase 7 — Enforcement and documentation

**Goal**: Convention lint prevents drift. Tool docs complete.

**Steps**:

1. Create `scripts/check/lint-scripts.sh` — validates:
   - All `.sh` files use `#!/usr/bin/env bash`
   - All `.sh` files have `set -o errexit/nounset/pipefail`
   - All `.sh` files in `scripts/` use kebab-case naming
   - All `.sh` files have header comment in first 5 lines
   - No `.sh` files in flat `scripts/` (must be in subdirectory)

2. Add `make lint-scripts` target, include in `make check`.

3. Create `docs/tools/<toolname>.md` for each pinned tool:

   ```text
   docs/tools/
   ├── shell-scripts.md     # Already exists
   ├── clang-format.md
   ├── clang-tidy.md
   ├── cppcheck.md
   ├── doxygen.md
   ├── gitleaks.md
   ├── pmccabe.md
   ├── semgrep.md
   └── shellcheck.md
   ```

   Each file: version, purpose, usage, configuration, what works/doesn't, troubleshooting.

4. Update `docs/README.md` to link to `docs/tools/`.

5. Run `make index` to update INDEX.md.

**Validate**: `make lint-scripts && make check && make index`

**Branch**: `adr-44/enforcement`

---

### Phase summary

| Phase | Branch | Risk | Effort | Key change |
|-------|--------|------|--------|------------|
| 1 | `adr-44/version-pinning` | Low | Small | `versions.env`, update `setup.sh` |
| 2 | `adr-44/shellcheck` | Low | Medium | ShellCheck all scripts, fix issues |
| 3 | `adr-44/script-reorg` | Low | Medium | Move/rename to subdirectories |
| 4 | `adr-44/extract-logic` | Medium | Medium | Extract 5 targets to scripts |
| 5 | `adr-44/self-doc` | Low | Medium | Auto help, progress, check-deps |
| 6 | `adr-44/ci-proxy` | Medium | Medium | Simplify CI, pin OS/actions |
| 7 | `adr-44/enforcement` | Low | Medium | Lint script, tool docs |

### Dependencies

```text
Phase 1 (versions.env)
  ├── Phase 2 (ShellCheck) — needs shellcheck in versions.env
  │    └── Phase 3 (reorg) — scripts must pass ShellCheck before moving
  │         └── Phase 4 (extract) — new scripts go to organized dirs
  │              └── Phase 5 (self-doc) — help comments on final targets
  └── Phase 6 (CI) — can start after Phase 1, parallel to 3–5
Phase 7 (enforcement) — after Phase 4 (validates final structure)
```

Phases 1→2→3→4→5 are sequential. Phase 6 can run in parallel after Phase 1.
Phase 7 runs last.

---

## Learnings (added during implementation)

### Git hooks follow the same extraction pattern

The `.config/pre-commit` hook was a flat shell script with inline logic — the same
anti-pattern we're fixing in the Makefile. Git hooks now live in `scripts/git/` and
follow the same conventions as all other scripts:

| Hook | Source | Delegates to |
|------|--------|-------------|
| `pre-commit` | `scripts/git/pre-commit.sh` | inline (short) |
| `pre-push` | `scripts/git/pre-push.sh` | `scripts/dev/prepush.sh` |

`make hooks` and `make setup` install both hooks. The `scripts/git/` directory is a
peer of `scripts/check/`, `scripts/dev/`, etc.

### Markup linters don't need Node or Python

- **yamllint**: Python-based but brew/apt handle the dependency transparently
- **rumdl**: Rust single-binary markdown linter (71 rules, zero runtime deps)
  - Chosen over markdownlint-cli2 (Node.js) to avoid adding Node as a project dependency
  - Config: `.config/rumdl.toml` (rumdl natively supports the `.config/` convention)

### Pipeline ordering follows shift-left principle

`make check` reordered: fastest no-build checks first (format, yamllint, markdownlint,
lint-makefile), then build-dependent analysis, then tests, then SAST, then metrics.

### lint-makefile.sh enforces the extraction rule

`scripts/check/lint-makefile.sh` counts physical shell lines per target (excluding
`@echo` and `$(MAKE)` calls). Added to `make check` to prevent regression.

### Phase 2 (ShellCheck) can be deferred

ShellCheck hygiene is valuable but not a blocker for script reorganization or extraction.
New scripts were written correctly from the start. ShellCheck can be a cleanup pass.
