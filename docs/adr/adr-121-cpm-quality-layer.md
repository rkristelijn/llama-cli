---
summary: Adopt CPM (Compliance Process Management) as the shared quality layer across all repos, replacing ad-hoc scripts with a universal framework that provides gamified CMMI levels, shared TUI output, and language-agnostic quality gates.
status: accepted
---

# ADR-121: Adopt CPM as Universal Quality Layer

## Context

llama-cli has grown 120+ scripts with ad-hoc output formatting, custom lint checks, and project-specific quality gates. The same patterns (pre-commit hooks, ADR enforcement, test coverage, complexity checks) are duplicated across sibling repos (workspace-tui, show-master, cpm itself). Each repo reinvents:

- Terminal output formatting (colors, progress, tables)
- Quality gate definitions (what to check at each stage)
- CMMI-level tracking (manual audits in ADR-048)
- Research freshness, slop detection, portability checks

Meanwhile, `../cpm` already has `lib/shell/ui.sh`, `lib/shell/log.sh`, `lib/shell/table.sh`, and a level-based quality model. It was originally "C Package Manager" but the vision has evolved.

## Decision

Adopt **CPM (Compliance Process Management)** as the shared quality framework for all repos. llama-cli becomes a consumer; cpm becomes the provider.

### What CPM provides (to any repo, any language)

1. **Shared TUI library** — `lib/shell/ui.sh` for consistent output across all scripts
2. **Quality gates per CMMI level** — progressive checks unlocked as maturity grows
3. **Gamification** — score, level, progression tracking
4. **Language-agnostic checks** — ADR enforcement, commit conventions, research freshness, slop detection, portability
5. **Language-specific plugins** — C++ (clang-tidy, cppcheck), TypeScript (biome), Python (ruff), etc.

### Integration model

```text
repo/
├── cpm.toml              ← declares level, language, overrides
├── .cpm/                 ← cached cpm library (gitignored or vendored)
│   ├── lib/shell/ui.sh
│   ├── lib/shell/log.sh
│   ├── lib/make/quality.mk
│   └── checks/           ← level-appropriate checks
└── Makefile              ← includes .cpm/lib/make/quality.mk
```

### Migration path for llama-cli

1. Add `cpm.toml` declaring level 3, language cpp
2. Source `.cpm/lib/shell/ui.sh` in all scripts (replace ad-hoc echo/printf)
3. Gradually move generic checks (slop, portability, research-freshness) into cpm
4. Keep llama-cli-specific checks (C++ complexity, mermaid tests) local

## CMMI Levels (CPM definition)

| Level | Name | What you get |
|-------|------|-------------|
| 0 | Anarchy | No checks, no process |
| 0.3 | Training Wheels | Pre-commit hooks, basic formatting, ADR template |
| 1 | Managed | Security scans, type checking, unit tests, commit conventions |
| 2 | Defined | Architecture docs, complexity limits, coverage gates, e2e tests |
| 3 | Quantitatively Managed | Metrics tracking, research freshness, slop detection, mutation testing |
| 4 | Optimizing | Trend analysis, auto-remediation, AI-assisted review |
| 5 | Excellence | Zero-defect targets, full automation, continuous improvement |

## Chosen Integration Strategy: Gradual Adoption with Fallback

After evaluating three approaches, we chose **Option C: Gradual overname**.

### Options considered

| Option | Mechanism | CI impact | Risk |
|--------|-----------|-----------|------|
| A. Symlink only | `lib/cpm → ../cpm/lib` | None (CI can't use it) | Contributors need cpm clone |
| B. Vendor/submodule | Copy or git submodule in repo | Works everywhere | Duplication or submodule pain |
| **C. Gradual (chosen)** | Symlink locally, Makefile fallback, release cpm later | Zero until cpm is stable | None — fully backwards-compatible |

### How it works

**Phase 1 (now):** CPM is a local dev-tool only. Symlink `lib/cpm` for developers who have it. Scripts source `lib/cpm/shell/ui.sh` with a fallback — if cpm is absent, scripts work exactly as before. CI and Makefile are unchanged.

**Phase 2 (when stable):** Release cpm as installable tool (one-liner curl install, like llama-cli itself). CI adds `install-cpm.sh` step. `cpm check` replaces `make check` in CI.

**Phase 3 (mature):** Makefile becomes a thin wrapper calling `cpm`. Local scripts that were migrated to cpm are removed from this repo.

### Fallback pattern for scripts

```bash
# Source cpm ui.sh if available, otherwise define minimal fallback
if [[ -f "${CPM_UI:-lib/cpm/shell/ui.sh}" ]]; then
  source "${CPM_UI:-lib/cpm/shell/ui.sh}"
fi
```

Scripts must work identically with or without cpm present.

### First-time-right output principle

One run gives you everything to act on. No re-runs, no `--verbose`, no "check the log".

| Result | What you see |
|--------|-------------|
| ✓ pass | Name + timing |
| ⚠ warning | Name + timing + what + where + why + fix suggestion |
| ✗ error | Name + timing + what + where + why + exact command to fix |

Every check finding is a structured record:

```text
{
  severity: "warning",
  check: "complexity",
  file: "src/config/config.cpp",
  line: 78,
  rule: "cyclomatic-complexity",
  message: "load_dotenv complexity=49 (max 10)",
  fix: "Split into smaller functions or annotate: // skip-complexity"
}
```

The renderer always shows actionable context — not just pass/fail. Compact but complete.

### Checks registry (datamodel)

Every check is declared with its metadata. The runner uses this to decide what to run, how to interpret output, and when to block.

```toml
[[checks]]
name = "format-code"
phase = "implement"
tool = "clang-format"
command = "scripts/fmt/format-code.sh"
triggers = ["src/**/*.cpp", "src/**/*.h"]
scope = "changed"             # only run on changed files
autofix = true
severity = "error"            # always fix
timeout = 30

[[checks]]
name = "complexity"
phase = "implement"
tool = "pmccabe"
command = "scripts/lint/check-complexity.sh"
triggers = ["src/**/*.cpp"]
scope = "changed"
autofix = false
severity = "warning"
timeout = 30
threshold = 10

[[checks]]
name = "slop"
phase = "implement"
tool = "grep"
command = "scripts/lint/check-slop.sh"
triggers = ["src/**/*.cpp", "src/**/*.h"]
scope = "diff"                # only the git diff, not full files
autofix = false
severity = "info"
timeout = 10

[[checks]]
name = "dead-code"
phase = "design"
tool = "custom"
command = "scripts/lint/check-dead-code.sh"
triggers = ["src/**"]
scope = "full"                # always scan everything
autofix = false
severity = "warning"
timeout = 60

[[checks]]
name = "shellcheck"
phase = "implement"
tool = "shellcheck"
command = "scripts/lint/check-scripts.sh"
triggers = ["scripts/**/*.sh"]
scope = "changed"
severity = "warning"
timeout = 60

[checks.shellcheck.mapping]
# Tool-specific codes → cpm severity
"SC2264" = "error"            # infinite recursion
"SC1090" = "info"             # can't follow dynamic source
"SC2034" = "info"             # unused var in sourced context
"*" = "warning"               # default for unmapped codes
```

### Incremental execution strategy

Not every check needs to run on every change. The runner decides based on triggers and scope.

**Scope types:**

| Scope | Meaning | Example |
|-------|---------|---------|
| `changed` | Only files in the delta | format, lint, complexity |
| `diff` | Only the git diff content | slop detection |
| `full` | Always scan everything | dead-code, xref, traceability |
| `affected` | Changed files + their dependents | clang-tidy (includes) |

**Decision flow:**

```text
1. git diff --name-only → which files changed?
2. For each check: do triggers match any changed file?
   No  → skip entirely
   Yes → check scope:
         changed  → pass file list to script
         diff     → pass diff content
         full     → run without args
         affected → resolve deps, pass expanded list
3. Check cache: file hash unchanged? → skip
4. Run check, record result + hash
```

**Execution tiers:**

```toml
[run.tiers]
# Tier 1: instant (<5s) — pre-commit, AI auto-fix loop
fast = ["format-*", "file-size", "slop"]

# Tier 2: thorough (<60s) — pre-push
normal = ["lint-*", "complexity", "shellcheck", "test-unit"]

# Tier 3: exhaustive (minutes) — CI only, pre-PR
full = ["dead-code", "xref", "coverage", "mutation", "e2e"]
```

**Performance mechanisms:**

| Mechanism | Saving | How |
|-----------|--------|-----|
| Skip irrelevant checks | 50-80% | Only YAML changed? Skip all C++ checks |
| Delta file list | 30-50% | clang-format on 3 files, not 200 |
| Result cache (hash→result) | 90%+ | No change = no re-check |
| Parallel execution | 2-4x | Independent checks run simultaneously |
| Timeout | Prevents hangs | Kill after N seconds, report as error |

**Cache model:**

```text
.tmp/cache/<check>/<file-path-hash>.result
  → contains: severity, message, file hash
  → invalidated when: file hash changes OR check config changes
```

### Production-proven patterns

Based on real-world CI/CD component ecosystems at scale (50+ components, multi-language):

**Output directory structure:**

```text
.tmp/
├── reports/              ← JUnit XML (CI artifact path)
│   ├── complexity-junit.xml
│   └── lint-code-junit.xml
├── timings.jsonl         ← timing + trend data
├── cache/                ← file hash → result cache
└── *.log                 ← tee output per check
```

CI configures artifact upload from `.tmp/reports/`. Everything in one place.

**Component structure per check:**

```text
checks/<name>/
├── check.sh              ← core logic (portable, no CI deps)
├── check-test.sh         ← self-test (validates the check works)
├── defaults.toml         ← default config (overridable via cpm.toml)
└── README.md             ← what it checks, why, how to fix
```

**JUnit XML output pattern** (proven to work across all CI providers):

```xml
<testsuites>
  <testsuite name="cpm-complexity" tests="3" failures="1">
    <testcase classname="src/config/config.cpp" name="load_dotenv" file="src/config/config.cpp">
      <failure message="complexity=49 (max 10)" type="warning">
        Split into smaller functions or annotate: // skip-complexity
      </failure>
    </testcase>
    <testcase classname="src/repl/repl.cpp" name="run_repl" file="src/repl/repl.cpp"/>
  </testsuite>
</testsuites>
```

Key fields: `classname` = file, `name` = function/rule, `failure.message` = what's wrong, CDATA = how to fix.

**Nix as first-class environment:**

```nix
# flake.nix — reproducible dev environment
{
  devShells.default = pkgs.mkShell {
    packages = [ clang-tools cppcheck pmccabe shellcheck ];
  };
}
```

This maps to cpm's `install-mode = "nix"` — the most reproducible option.

### Output layer: separation of data and presentation

**Unified severity levels** (every tool adapter maps to these):

| Level | Meaning | Blocks build? |
|-------|---------|---------------|
| `error` | Must fix, breaks the gate | Yes |
| `warning` | Should fix, not urgent | Only at CMMI level 4+ |
| `info` | Informational, no action needed | Never |

Each tool has an adapter that translates tool-specific output:

```text
shellcheck SC2264 (error)   → cpm error
shellcheck SC1090 (warning) → cpm info (known, non-actionable)
pmccabe complexity > 10     → cpm warning (level 3) or error (level 4)
clang-format diff           → cpm error (always fixable)
```

**Run modes:**

```toml
# cpm.toml
[run]
mode = "collect"  # collect | fail-fast
```

| Mode | Behavior | Use case |
|------|----------|----------|
| `fail-fast` | Stop at first error | AI auto-fix loop, quick feedback |
| `collect` | Run everything, report all at end | Pre-push, CI, full review |

In `collect` mode, the wrapper accumulates all results and exits non-zero only if any `error` was found. Warnings and info are shown but don't block.

Checks produce **structured data**. The UI layer decides how to present it. Output format is a provider.

```text
┌──────────────┐     ┌──────────────────┐     ┌──────────────┐
│  Check runs  │ ──► │  Structured data  │ ──► │  Renderer    │
│  (scripts)   │     │  (JSONL events)   │     │  (provider)  │
└──────────────┘     └──────────────────┘     └──────────────┘
                                                     │
                                          ┌──────────┼──────────┐
                                          ▼          ▼          ▼
                                       Terminal   JUnit XML   JSON API
                                       (human)   (CI tools)  (dashboards)
```

**Output providers:**

| Provider | Format | Consumer |
|----------|--------|----------|
| `terminal` (default) | Colored text, spinners, progress bars | Developer in terminal |
| `compact` | One line per check, no animation | CI logs, piped output |
| `junit` | JUnit XML (`TEST-cpm.xml`) | GitHub Actions, GitLab, Jenkins, SonarCloud |
| `json` | JSON lines (`.tmp/timings.jsonl`) | Dashboards, trend analysis |
| `tap` | TAP (Test Anything Protocol) | Legacy test harnesses |
| `silent` | Only errors to stderr | Scripted automation |

**Configuration:**

```toml
# cpm.toml
[ui]
mode = "auto"           # auto | spinner | progress | checklist | compact | silent
output = ["terminal"]   # can emit multiple: ["terminal", "junit"]

# Auto-detect:
#   TTY + interactive → checklist with spinners
#   CI / pipe         → compact + junit
#   --quiet           → silent
```

**JUnit integration** (for CI tools that parse test results):

```xml
<!-- .tmp/TEST-cpm.xml -->
<testsuite name="cpm" tests="12" failures="1" time="4.2">
  <testcase name="format-code" time="1.2"/>
  <testcase name="lint-scripts" time="0.8"/>
  <testcase name="file-size" time="0.3">
    <failure message="src/foo.cpp: 650 net lines (max 600)"/>
  </testcase>
</testsuite>
```

This integrates with:

- GitHub Actions → test summary in PR
- GitLab CI → test report tab
- Jenkins → test trend graphs
- SonarCloud → external analysis import

**Design principle:** checks never `echo` directly. They call `print_*` functions which route to the active renderer(s). This means the same check run can simultaneously show a spinner to the developer AND write JUnit XML for CI.

### Tool management: consent-based installation

CPM never installs tools without explicit user consent. Developers choose their preferred strategy.

**Install modes** (configurable per user, like `git pull.rebase`):

```toml
# cpm.toml — project default (suggestion for newbies)
[tools]
install-mode = "ask"  # ask | auto | skip | nix | docker
```

| Mode | Behavior | For whom |
|------|----------|----------|
| `ask` (default) | Prompt per tool: global/project/skip/never | Newbies, cautious devs |
| `auto` | Install missing tools silently | Experienced devs who trust the config |
| `skip` | Never install, warn and skip check | CI without setup step, minimal envs |
| `nix` | Use `flake.nix` / `devbox.json` | Nix users, pure reproducibility |
| `docker` | Run checks in container | Teams wanting zero local deps |

**The prompt (ask mode):**

```text
$ cpm check
  semgrep 1.67.0 not found.
  Install? [g]lobal (brew) / [p]roject (.cpm/bin/) / [s]kip / [n]ever ask
  > g
  ✓ installed semgrep 1.67.0 via brew
```

**Three scopes:**

| Scope | Location | Versioned | Shared |
|-------|----------|-----------|--------|
| Global | brew/apt (`/usr/local/bin`) | No (system-wide) | All projects |
| Project | `.cpm/bin/` (gitignored) | Yes (pinned in cpm.toml) | This repo only |
| Nix | `/nix/store/` | Yes (flake.lock) | Per flake |

**User preferences** are stored per-user, never committed:

```bash
# .cpm/preferences.env (gitignored)
CPM_INSTALL_MODE=ask
SEMGREP_INSTALL=global
GITLEAKS_INSTALL=project
TRIVY_INSTALL=skip
```

**Pinned versions** are committed (source of truth for the project):

```toml
# cpm.toml
[tools.versions]
semgrep = "1.67.0"
gitleaks = "8.18.2"
shellcheck = "0.10.0"
clang-format = "14"
```

**Design principles:**

- Never install without consent (not a virus)
- Default = `ask` with sensible suggestion (global for common tools)
- Experienced devs set `auto` once and forget
- Project-local installs don't pollute the system
- `cpm doctor` shows what's installed, what's missing, what version mismatches

### Configuration location strategy

Project root stays clean. Only project manifests live in root:

```text
repo/
├── cpm.toml              ← project manifest (like Cargo.toml, package.json)
├── Makefile              ← build orchestration
├── CMakeLists.txt        ← build system
└── .config/              ← ALL tool configs
    ├── .clang-format
    ├── .clang-tidy
    ├── rumdl.toml
    ├── yamllint.yml
    ├── versions.env
    └── ...
```

**Rule:** If a tool supports `--config <path>`, put its config in `.config/` and pass the path. CPM wraps tools with the correct config path automatically.

Config location is itself a provider:

```toml
# cpm.toml
[config]
dir = ".config"           # where tool configs live
# Tools that can't take --config get a root symlink (gitignored)
```

CPM resolves config for each tool:

| Tool | Native location | CPM location | How |
|------|----------------|--------------|-----|
| clang-format | `.clang-format` | `.config/.clang-format` | `--style=file:.config/.clang-format` |
| clang-tidy | `.clang-tidy` | `.config/.clang-tidy` | `--config-file=.config/.clang-tidy` |
| yamllint | `.yamllint` | `.config/yamllint.yml` | `-c .config/yamllint.yml` |
| rumdl | `rumdl.toml` | `.config/rumdl.toml` | `--config .config/rumdl.toml` |
| shellcheck | `.shellcheckrc` | `.config/shellcheckrc` | root symlink (no --config) |
| doxygen | `Doxyfile` | `.config/Doxyfile` | `doxygen .config/Doxyfile` |

Tools that don't support `--config` get a root symlink → `.config/X` (symlink is gitignored).

### Distribution: embedded copy (not symlink)

CPM is committed as a copy in `lib/cpm/` — not a symlink, not a submodule.

```text
lib/cpm/
├── cpm                    ← compiled binary (orchestrator)
├── VERSION                ← pinned version for update tracking
├── shell/
│   ├── ui.sh              ← TUI module (sourced by scripts)
│   ├── log.sh             ← audit trail module
│   └── table.sh           ← table output module
├── make/                  ← Makefile includes (Phase 2)
└── templates/             ← scaffolding templates (Phase 3)
```

Why copy over symlink:

- Symlinks to `../../cpm/lib` break for anyone without the cpm repo cloned adjacent
- CI runners don't have `../cpm` — a copy works everywhere
- Each file is an independently updatable module (like DLLs)
- `VERSION` file tracks which cpm release is embedded

### Multi-arch binary strategy

The `cpm` binary is architecture-specific. For CI (Ubuntu x64):

- **Phase 1 (now):** Only shell libs are used — pure bash, works everywhere
- **Phase 2 (when cpm orchestration needed in CI):** Cross-compile and commit both:
  - `lib/cpm/cpm-darwin-arm64`
  - `lib/cpm/cpm-linux-x64`
  - Scripts auto-select: `CPM="lib/cpm/cpm-$(uname -s | tr A-Z a-z)-$(uname -m)"`

### Updating cpm

```bash
# Manual update (for now)
cp -R ../cpm/lib lib/cpm && cp ../cpm/cpm lib/cpm/cpm

# Future: make update-cpm (pulls latest from release)
```

## Architecture Vision: scripts/ → empty

The end goal is that `scripts/` in consumer repos is empty (or contains only project-specific logic). All generic quality checks live in cpm. The repo only declares *what* it wants via `cpm.toml`.

### Core vs Providers vs Hooks

```text
┌─────────────────────────────────────────────────────┐
│                   CPM Core                           │
│  (process engine, V-model phases, severity, scoring)│
└──────────┬──────────────────────────┬───────────────┘
           │                          │
    ┌──────▼──────┐           ┌───────▼───────┐
    │  Providers  │           │    Hooks      │
    │  (pluggable)│           │  (extensible) │
    └─────────────┘           └───────────────┘
```

**Core** — the process engine. Defines V-model phases, runs checks in order, tracks severity, computes score. Language-agnostic. Never changes per repo.

**Providers** — pluggable backends for CI, caching, hosting:

| Provider type | Examples | Interface |
|---------------|----------|-----------|
| CI | GitHub Actions, GitLab CI, Forgejo, Jenkins | `provider_run_pipeline()` |
| Cache | GitHub cache, local fs, S3, GitLab cache | `provider_cache_get()`, `provider_cache_set()` |
| Hosting | GitHub, GitLab, Codeberg, self-hosted | `provider_create_pr()`, `provider_comment()` |
| Review | CodeRabbit, SonarCloud, local AI | `provider_review()` |

Adding a new CI host = writing one provider file, not changing core.

**Hooks** — extension points for proprietary/custom logic:

| Hook | V-model phase | When |
|------|---------------|------|
| `pre-format` | Implement | Before auto-formatting |
| `post-format` | Implement | After auto-formatting |
| `pre-lint` | Implement | Before linting |
| `post-lint` | Implement | After linting |
| `pre-build` | Build | Before compilation |
| `post-build` | Build | After successful build |
| `pre-test` | Test | Before test run |
| `post-test` | Test | After test run |
| `pre-release` | Release | Before version bump |
| `post-release` | Release | After release |

Hooks allow attaching proprietary tools (license scanners, internal security tools) without forking cpm.

### V-Model Phases (functional names)

Industry-standard phase names mapped to cpm check categories:

```text
         DEFINE                              VERIFY
         ──────                              ──────
    ┌─ Motivation ─┐                  ┌─ Acceptance ─┐
    │  (why)       │                  │  (criteria)  │
    ├─ Design ─────┤                  ├─ System ─────┤
    │  (how)       │                  │  (e2e)       │
    ├─ Implement ──┤                  ├─ Integration ┤
    │  (code)      │                  │  (component) │
    └─ Unit ───────┘                  └─ Unit ───────┘
              │                          │
              └──────── BUILD ───────────┘
```

| Phase | cpm category | Checks |
|-------|-------------|--------|
| **Motivation** | `process` | ADR exists, acceptance criteria defined |
| **Design** | `structure` | File size, dir limits, dead docs, xref |
| **Implement** | `format` + `code` | Formatting, linting, complexity, slop |
| **Build** | `build` | Compile, link, no warnings |
| **Unit** | `test.unit` | Unit tests pass, coverage gate |
| **Integration** | `test.integration` | Component tests, mock tests |
| **System** | `test.e2e` | End-to-end tests, live tests |
| **Acceptance** | `test.acceptance` | Feature coverage markers, manual gate |
| **Release** | `release` | Version bump, changelog, no regressions |

### Severity Progression per CMMI Level

The same check can be a warning at one level and an error at the next. The V-model grows stricter as maturity increases:

| Check | Level 0-1 | Level 2 | Level 3 | Level 4+ |
|-------|-----------|---------|---------|----------|
| File size > limit | — | warning | error | error |
| Missing ADR | — | — | warning | error |
| Slop detected | — | — | warning | error |
| Coverage < 60% | — | warning | error | error |
| Research stale | — | — | info | warning |
| Dead code | — | — | warning | error |
| Complexity > 10 | — | info | warning | error |

Configured in `cpm.toml`:

```toml
[severity]
# Override default severity for this repo
filesize = "error"        # strict from day one
slop = "warning"          # not blocking yet
research-freshness = "info"  # just informational
```

### Caching (provider pattern)

Caching is a provider, not hardcoded to GitHub:

```toml
[providers]
ci = "github"           # or "gitlab", "forgejo", "local"
cache = "filesystem"    # or "github", "s3", "gitlab"

[cache]
dir = ".tmp/cache"      # local cache dir
key = "build-${hash(src/**,CMakeLists.txt)}"
```

The cache provider interface:

```bash
# Any provider implements these:
provider_cache_get() { local key="$1"; ... }
provider_cache_set() { local key="$1" path="$2"; ... }
provider_cache_exists() { local key="$1"; ... }
```

Built-in providers:

- `filesystem` — local `.tmp/cache/` (works everywhere, no setup)
- `github` — `actions/cache` (CI only)
- `gitlab` — GitLab CI cache directive
- `s3` — remote shared cache (team use)

## Consequences

### Developer experience: inclusivity by design

Every target has two names: a short alias for power users and a human-readable name for newcomers.

```makefile
# Power user          # Newcomer              # Same thing
make cpm-fast         make quick-check        # quick quality check
make cpm              make full-check         # full check before push
make gpr              make create-pr          # create PR
make gps              make status             # CI status
make gprr             make ready              # mark ready for review
make gpc              make review             # full review
```

`make workflow` shows the numbered daily flow — no guessing which of 50+ targets to use.

**Principle:** if a command name requires tribal knowledge, add a readable alias. Both work, neither is deprecated.

### V-model workflow integration

Development follows a V-shape: define (left, getting specific) → build (bottom) → verify (right, getting broad).

```text
V-DOWN (define)                              V-UP (verify)
───────────────                              ──────────────
1. Motivation   ←─────────────────────────→  Acceptance
2. Options      ←─────────────────────────→  System (e2e)
3. Design       ←─────────────────────────→  Integration
4. Contract     ←─────────────────────────→  Unit
                         5. Build
```

**Each level has a quality gate:**

| V-level | V-DOWN activity | V-UP gate | make target |
|---------|----------------|-----------|-------------|
| 1 | Motivation: why, value, ADR | Acceptance: criteria met, PR approved | `make review` |
| 2 | Options: trade-offs, alternatives | System: e2e, live test | `make cpm-full` |
| 3 | Design: architecture, flow | Integration: components together | `make full-check` |
| 4 | Contract: API, interfaces, mocks | Unit: individual functions | `make quick-check` |
| 5 | Implement: write code | Build compiles | `make build` |

**When to push:**

| Intent | V-UP level required | How |
|--------|-------------------|-----|
| Safe work (backup) | None | `git push` (WIP branch, no gate) |
| Ready for CI | 4 (unit green) | `make quick-check && git push` |
| Ready for review | 3 (integration green) | `make full-check && git push` |
| Ready for merge | 1 (acceptance) | CI + review + `make review` |

**`make next` (future):**

Tracks where you are in the V-model and suggests the next step:

```text
$ make next
  Current: V-DOWN level 3 (design)
  ✓ ADR-121 exists (motivation)
  ✓ Options documented (options)
  → Design: define interfaces for registry parser
  
  Next steps:
    1. Write contract (interfaces, mocks)
    2. Write unit tests
    3. Implement
    4. make quick-check
    5. Push
```

Implementation: reads git state (branch name, changed files, test results) to infer position. Could also be a prompt/agent that guides the developer.

### Reality check: current state assessment

| Principle | Status | Issue |
|-----------|--------|-------|
| DRY | ❌ | 79 Makefile targets duplicate what cpm.toml already defines |
| KISS | ⚠️ | Two systems (Makefile + cpm) orchestrate the same checks |
| SOLID (SRP) | ⚠️ | Makefile does orchestration AND check definition |
| Pluggable | ✅ | cpm.toml registry is extensible |
| Performance | ✅ | Delta detection, timing, trend analysis work |
| Framework as intended | ❌ | cpm is the framework but Makefile still does the work |
| Inclusivity | ✅ | Human-readable aliases, `make workflow` |

**Root cause:** The Makefile grew organically (120+ scripts) before cpm existed. Now cpm.toml is the source of truth for checks, but the Makefile still has individual targets for each one.

**Target state:**

```text
Current:  Makefile (511 lines) → scripts/
          cpm.toml (19 checks) → scripts/   ← duplicate path

Target:   Makefile (thin) → cpm → scripts/
          Only workflow aliases + build/run/install in Makefile
```

**Migration path (backwards compatible):**

1. Phase A (done): cpm.toml registry + `make cpm-fast` / `make quick-check`
2. Phase B (next): individual Makefile targets become one-liners calling cpm
3. Phase C (later): remove individual targets, only workflow aliases remain

The Makefile's role shrinks to: build, run, install, workflow aliases. Everything else is cpm's job.

### Implicit decisions captured

| Decision | Rationale |
|----------|-----------|
| `cpm.toml` in root, tool configs in `.config/` | Project manifest = root (like Cargo.toml), tool configs = hidden |
| Shell scripts for checks, not the binary | Binary doesn't support [[checks]] yet; shell is portable and debuggable |
| `init.sh` as single source line | One line per script, fallback logic in one place, not 108 copies |
| `set +o pipefail` around tee in run.sh | grep-based scripts return exit 1 on no match; tee propagates this |
| Severity warning = non-blocking | Only `error` blocks the build; warnings are informational until CMMI level 4 |
| Delta detection via git diff | No file watcher, no daemon — simple, stateless, works in CI |
| Timer uses temp files not associative arrays | macOS /bin/bash is 3.2 (no `declare -A`); temp files work everywhere |
| JUnit XML in `.tmp/reports/` | Single output dir, CI configures artifact upload path once |
| Checks run sequentially (not parallel) | Simpler output, no interleaving; parallel is a future optimization |

## Consequences

- Scripts get consistent output via shared TUI library
- Quality checks become portable across repos
- New repos start at level 0.3 and grow organically
- llama-cli's existing checks become the reference implementation for cpm level 3
- Reduces duplication across repos by ~60% for generic checks

## References

- @see ../cpm/lib/shell/ui.sh (existing TUI library)
- @see docs/adr/adr-003-v-model-workflow.md (V-model phases: Define → Build → Test → Audit → Release)
- @see docs/adr/adr-048-quality-framework.md (current CMMI implementation)
- @see docs/adr/adr-101-v-model-quality-gates.md (quality gates per phase)
- @see docs/adr/adr-119-slop-detection.md (candidate for cpm extraction)
- @see docs/adr/adr-120-research-freshness.md (candidate for cpm extraction)
- @see docs/adr/adr-122-ci-caching-portability.md (cache provider strategy)
