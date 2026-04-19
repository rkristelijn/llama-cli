# Make Command Tree

This document provides a visual overview of all `make` commands and how they orchestrate the project's execution and quality gates.

## 🌳 Command Hierarchy

### 🏗️ Core (Build & Base)

```text
all (Internal target)
└── check-deps (scripts/lint/check-deps.sh)
└── cmake (build configuration)
└── build (compilation)

build
└── all

setup
└── scripts/dev/setup.sh

clean (removes build artifacts)
```

### 🧹 Aggregators (Simplified Interface)

These are the primary commands for daily development.

```text
format (Auto-fix everything)
├── format-code (scripts/fmt/format-code.sh)
├── format-md (scripts/fmt/format-md.sh)
├── format-yaml (scripts/fmt/format-yaml.sh)
└── format-scripts (scripts/fmt/format-scripts.sh)

lint (Passive quality checks)
├── lint-code (aggregator)
│   ├── lint-format-code (clang-format --dry-run)
│   └── lint-cppcheck (scripts/lint/lint-code.sh)
├── lint-md (scripts/lint/lint-md.sh)
├── lint-yaml (scripts/lint/lint-yaml.sh)
├── lint-makefile (scripts/lint/check-makefile.sh)
├── lint-scripts (scripts/lint/check-scripts.sh)
├── tidy (scripts/lint/run-tidy.sh)
├── complexity (scripts/lint/check-complexity.sh)
├── comment-ratio (scripts/lint/check-comment-ratio.sh)
└── docs (doxygen)

test (Run all tests)
├── test-unit (scripts/test/run-unit.sh)
└── e2e (scripts/test/run-e2e.sh)

check (Full Quality Gate)
├── lint (Aggregator)
├── test (Aggregator)
└── sast (Aggregator)
    ├── sast-security (semgrep)
    └── sast-secret (gitleaks)
```

### 🚀 Execution & Feedback

```text
start (alias: s, run, r)
└── all
    └── [executing ./build/llama-cli]

quick
└── all
└── scripts/dev/quick.sh (unit tests + comment ratio)

live
└── all
└── e2e/test_live.sh (integration test with LLM)
```

### 🛠️ Development & Git Hooks

```text
hooks (installs pre-commit and pre-push)
└── scripts/git/pre-commit.sh
└── scripts/git/pre-push.sh

pre-commit hook
└── scripts/git/precommit-check.sh (calls make format, index, sast-secret)

pre-push hook
└── scripts/git/prepush-check.sh (calls make check)
```

## 📝 Design Principles

- **Separation of Concerns**: The `Makefile` orchestrates, while scripts in `scripts/` execute specific tools.
- **Passive vs Active**: `make lint` is strictly passive (no changes), while `make format` is active (auto-fixes).
- **Consistency**: The same targets are used locally via hooks and in the CI pipeline.
