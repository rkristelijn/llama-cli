BUILD_DIR = build
BINARY = $(BUILD_DIR)/llama-cli

# FULL=1 disables smart mode for exhaustive checks (e.g. clang-tidy)
FULL ?= 0

.DEFAULT_GOAL := help

.PHONY: all build clean run start s log test t test-unit e2e check full-check check-ai \
	format format-code format-yaml format-md format-scripts \
	lint lint-code lint-yaml lint-md lint-makefile lint-scripts \
	tidy complexity comment-ratio docs sast sast-secret sast-security \
	coverage coverage-report todo quick index setup install hooks bench features fuzz \
	gh-pipeline-status gpls gh-pr-status gps gh-create-pr gpr \
	gh-download-issues gdi gh-pr-feedback gpf create-issue \
	bump major minor patch

##@ Getting Started

setup: ## Install all dependencies
	@bash scripts/dev/setup.sh

build: all ## Build the project

start: all ## Build and run the REPL (alias: s)
	@./$(BINARY) $(ARGS)
s: start

run: start ## Alias for start
r: run

install: all ## Install to /usr/local/bin
	@cp $(BINARY) /usr/local/bin/llama-cli
	@echo "Installed to /usr/local/bin/llama-cli"

hooks: ## Install git hooks (pre-commit, commit-msg, pre-push)
	@cp scripts/git/pre-commit.sh .git/hooks/pre-commit
	@cp scripts/git/commit-msg.sh .git/hooks/commit-msg
	@cp scripts/git/pre-push.sh .git/hooks/pre-push
	@chmod +x .git/hooks/pre-commit .git/hooks/commit-msg .git/hooks/pre-push
	@echo "Git hooks installed (pre-commit, commit-msg, pre-push)."

clean: ## Remove build artifacts
	rm -rf $(BUILD_DIR) llama-cli

##@ Aggregators

format: format-code format-md format-yaml format-scripts ## Auto-format all files

lint: lint-code lint-md lint-yaml lint-makefile lint-scripts tidy complexity comment-ratio docs file-size ## Run all passive checks

test: build test-unit e2e ## Run all tests (builds first)

check: build lint test sast ## Full quality gate (CI/pre-push)

full-check: ## Run exhaustive quality checks (FULL=1)
	@$(MAKE) FULL=1 check

check-ai: ## Run checks with condensed output
	@$(MAKE) -s check 2>&1 | grep -E "^\s*([0-9]+|src/|==>|\[|FAIL|All|knownCondition|always false|too many|warning:|error:)" | grep -v "^$$"

##@ Formatting

format-code: ## Format C++ code (clang-format)
	@bash scripts/fmt/format-code.sh

format-md: ## Format Markdown files (rumdl)
	@bash scripts/fmt/format-md.sh

format-yaml: ## Format YAML files (trailing whitespace)
	@bash scripts/fmt/format-yaml.sh

format-scripts: ## Format shell scripts (shfmt)
	@bash scripts/fmt/format-scripts.sh

##@ Linting

lint-code: lint-format-code lint-cppcheck ## Lint C++ code (format + cppcheck)

lint-format-code: ## Check C++ formatting (no changes)
	@echo "==> checking C++ formatting..."
	@find src -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run -Werror --style=file:.config/.clang-format
	@echo "  [done] lint-format-code"

lint-cppcheck: ## Run cppcheck static analysis
	@bash scripts/lint/lint-code.sh

lint-md: ## Lint Markdown files (rumdl)
	@bash scripts/lint/lint-md.sh

lint-yaml: ## Lint YAML files (yamllint)
	@bash scripts/lint/lint-yaml.sh

lint-makefile: ## Check Makefile conventions
	@bash scripts/lint/check-makefile.sh

lint-scripts: ## Check shell script conventions (shellcheck)
	@bash scripts/lint/check-scripts.sh

tidy: ## Run clang-tidy (smart: changed files only)
	@bash scripts/lint/run-tidy.sh $(if $(filter 1,$(FULL)),--full)

complexity: ## Check cyclomatic complexity (pmccabe)
	@bash scripts/lint/check-complexity.sh

comment-ratio: ## Show comment ratio per file
	@bash scripts/lint/check-comment-ratio.sh

docs: ## Check doxygen warnings
	@echo "==> checking doxygen..."
	@output=$$(doxygen .config/Doxyfile 2>&1) || { echo "doxygen failed"; exit 1; }; \
	echo "$$output" | grep "warning:" | grep -v "No output formats\|Unsupported xml\|falsely parses" && exit 1 || true
	@echo "  [done] docs"

file-size: ## Check source file sizes (ADR-061)
	@bash scripts/lint/check-file-size.sh

##@ Testing

test-unit: ## Run unit tests
	@bash scripts/test/run-unit.sh "$(BUILD_DIR)"
t: test-unit

e2e: ## Run end-to-end tests
	@bash scripts/test/run-e2e.sh "$(BUILD_DIR)" "$(BINARY)"

live: ## Integration test with real LLM
	@bash e2e/test_live.sh $(BINARY)

bench: ## Benchmark local Ollama models (see docs/model-bench.md)
	@bash scripts/test/bench-models.sh $(ARGS)

coverage: ## Build with coverage and run tests
	@bash scripts/test/run-coverage.sh "$(BUILD_DIR)"

coverage-report: coverage ## Show coverage summary per directory
	@bash scripts/test/report-coverage.sh "$(BUILD_DIR)"

quick: ## Fast feedback: build + unit tests + comment ratio
	@$(MAKE) build
	@bash scripts/dev/quick.sh "$(BUILD_DIR)"

features: ## List all test scenarios (feature spec)
	@for bin in test_repl test_annotations test_annotation test_config test_command test_json test_exec test_markdown test_logger test_trace; do \
		[ -f "$(BUILD_DIR)/$$bin" ] && ./$(BUILD_DIR)/$$bin --list-test-cases 2>/dev/null | grep "Scenario:" | sed 's/^    //'; \
	done

fuzz: ## Run annotation fuzzer (60s, requires llvm)
	@LLVM=$$(brew --prefix llvm 2>/dev/null || echo /usr/bin); \
	cmake -B build-fuzz -DENABLE_FUZZ=ON -DCMAKE_C_COMPILER="$$LLVM/bin/clang" -DCMAKE_CXX_COMPILER="$$LLVM/bin/clang++" > /dev/null && \
	cmake --build build-fuzz --target fuzz_annotation > /dev/null
	@echo "==> fuzzing annotation parser (60s)..."
	@./build-fuzz/fuzz_annotation -max_total_time=60 -print_final_stats=1

##@ Security

sast: sast-security sast-secret ## Run all SAST checks

sast-security: ## Run semgrep security scan
	@echo "==> running sast-security (semgrep)..."
	@if command -v semgrep >/dev/null; then \
		semgrep scan --config auto --error --quiet 2>&1 | grep -v "┌────\|Semgrep CLI\|└─────────────" || true; \
	else echo "  [skip] semgrep not installed"; fi
	@echo "  [done] sast-security"

sast-secret: ## Run gitleaks secret scan
	@echo "==> running sast-secret (gitleaks...)"
	@if command -v gitleaks >/dev/null; then \
		gitleaks detect --source . --log-level error --no-banner; \
	else echo "  [skip] gitleaks not installed"; fi
	@echo "  [done] sast-secret"

##@ Development

bump: ## Bump version (make bump PART=patch|minor|major)
	@bash scripts/dev/bump.sh "$(or $(PART),$(filter major minor patch,$(MAKECMDGOALS)))"

major minor patch:
	@true

log: ## View event logs
	@bash scripts/dev/log-viewer.sh $(ARGS)

todo: ## Show TODO items from docs and code
	@bash scripts/dev/todo.sh

index: ## Regenerate INDEX.md
	@bash scripts/dev/build-index.sh

summarize: ## Summarize file headers with Ollama (--dry-run supported)
	@bash scripts/dev/summarize-headers.sh $(ARGS)

precommit: ## Run pre-commit checks
	@bash scripts/git/precommit-check.sh

prepush: ## Run pre-push checks
	@bash scripts/git/prepush-check.sh

##@ GitHub

gh-pipeline-status: ## Show latest pipeline status (alias: gpls)
	@bash scripts/gh/pipeline-status.sh
gpls: gh-pipeline-status

gh-pr-status: ## Show failed PR jobs (alias: gps)
	@bash scripts/gh/pr-status.sh $(ARGS)
gps: gh-pr-status

gh-create-pr: ## Create pull request (alias: gpr)
	@bash scripts/gh/create-pr.sh
gpr: gh-create-pr

gh-download-issues: ## Download GitHub issues (alias: gdi)
	@bash scripts/gh/download-issues.sh
gdi: gh-download-issues

gh-pr-feedback: ## Show CodeRabbit feedback (alias: gpf)
	@bash scripts/gh/pr-feedback.sh
gpf: gh-pr-feedback

create-issue: ## Create issue (TITLE="..." DESC="...")
	@if [ -z "$(TITLE)" ] || [ -z "$(DESC)" ]; then \
		echo "Usage: make create-issue TITLE=\"My title\" DESC=\"My description\""; \
		exit 1; \
	fi
	@bash scripts/gh/create-issue.sh "$(TITLE)" "$(DESC)"

##@ Help

help: ## Show this help
	@awk 'BEGIN {FS = ":.*##"; printf "Usage:\n  make \033[36m<target>\033[0m\n"} \
		/^[a-zA-Z_0-9-]+:.*?##/ { printf "  \033[36m%-25s\033[0m %s\n", $$1, $$2 } \
		/^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) }' $(MAKEFILE_LIST)

# Internal targets
all: $(if $(filter 1,$(SKIP_DEPS)),,check-deps)
	@cmake -B $(BUILD_DIR) -S . > /dev/null
	@cmake --build $(BUILD_DIR) --target llama-cli > /dev/null
	@cp $(BINARY) .

check-deps:
	@bash scripts/lint/check-deps.sh

check-versions:
	@bash scripts/lint/check-versions.sh
