BUILD_DIR = build
BINARY = $(BUILD_DIR)/llama-cli

# FULL=1 disables smart mode for exhaustive checks (e.g. clang-tidy)
FULL ?= 0

# LOG=1 enables automatic tee to .tmp/<target>.log for any target
LOG ?= 1
ifdef LOG
ifneq ($(LOG),0)
# Wrap recipe output: tee to .tmp/<target>.log and print path at end
define log_footer
	@echo "  📄 log: .tmp/$@.log"
endef
SHELL := /bin/bash
.SHELLFLAGS := -o pipefail -c
export MAKE_LOG_DIR := .tmp
$(shell mkdir -p .tmp)
endif
endif

.DEFAULT_GOAL := help

.PHONY: all build clean run start s log test t test-unit e2e check check-fast check-all full-check mutation check-ai \
	format format-code format-yaml format-md format-scripts \
	lint lint-code lint-yaml lint-md lint-makefile lint-scripts lint-versions \
	tidy complexity comment-ratio docs file-size sast sast-secret sast-security sast-stegano sast-iac sast-trufflehog sast-grype sast-osv sast-checkov sast-codeql sbom consistency \
	coverage coverage-report todo quick index setup install hooks bench features fuzz update \
	sonar sonar-report trivi learn \
	gh-pipeline-status gpls gh-pr-status gps gh-create-pr gpr \
	gh-download-issues gdi gh-pr-feedback gpf create-issue \
	bump major minor patch

##@ Getting Started

setup: ## Install all dependencies
	@bash scripts/dev/setup.sh
	$(log_footer)

build: all ## Build the project
#      	   shellcheck           ✓
#          yamllint             ✓
#          rumdl                ✓
	@echo "  build                ✓"

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

kill: ## Kill running llama-cli instances
	@pkill -xf '(\./)?llama-cli|build/llama-cli' 2>/dev/null && echo "  killed llama-cli" || echo "  no llama-cli running"

##@ Aggregators

format: format-code format-md format-yaml format-scripts ## Auto-format all files

lint: lint-code lint-md lint-yaml lint-makefile lint-scripts lint-versions tidy complexity comment-ratio docs file-size consistency check-theme check-xref check-interactive-input ## Run all passive checks

test: build test-unit e2e ## Run all tests (builds first)

check-fast: build format ## Tier 1: format + build (AI auto-fix loop)
	@mkdir -p .tmp
	@$(MAKE) build format 2>&1 | tee .tmp/check-fast.log
	$(log_footer)

check: ## Tier 2: full quality gate (CI/pre-push)
	@mkdir -p .tmp
	@$(MAKE) build lint test sast 2>&1 | tee .tmp/check.log
	$(log_footer)

check-all: ## Tier 3: everything — exhaustive lint, mutation, SBOM, docs (alias: full-check)
	@mkdir -p .tmp
	@$(MAKE) FULL=1 build lint test sast mutation sbom todo summarize-safe index dead-code dead-docs duplication 2>&1 | tee .tmp/check-all.log
	$(log_footer)
full-check: check-all

mutation: ## Run mutation testing (Mull, slow — PR only)
	@bash scripts/test/run-mutation.sh
	$(log_footer)

check-ai: ## Run checks with condensed output
	@$(MAKE) -s check 2>&1 | grep -E "^\s*([0-9]+|src/|==>|\[|FAIL|All|knownCondition|always false|too many|warning:|error:)" | grep -v "^$$"

##@ Formatting

format-code: ## Format C++ code (clang-format)
	@bash scripts/fmt/format-code.sh
	$(log_footer)

format-md: ## Format Markdown files (rumdl)
	@bash scripts/fmt/format-md.sh
	$(log_footer)

format-yaml: ## Format YAML files (trailing whitespace)
	@bash scripts/fmt/format-yaml.sh
	$(log_footer)

format-scripts: ## Format shell scripts (shfmt)
	@bash scripts/fmt/format-scripts.sh
	$(log_footer)

##@ Linting

lint-code: lint-format-code lint-cppcheck ## Lint C++ code (format + cppcheck)

lint-format-code: ## Check C++ formatting (no changes)
	@echo "==> checking C++ formatting..."
	@find src -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run -Werror --style=file:.config/.clang-format
	@echo "  [done] lint-format-code"

lint-cppcheck: ## Run cppcheck static analysis
	@bash scripts/lint/lint-code.sh
	$(log_footer)

lint-md: ## Lint Markdown files (rumdl)
	@bash scripts/lint/lint-md.sh
	$(log_footer)

lint-yaml: ## Lint YAML files (yamllint)
	@bash scripts/lint/lint-yaml.sh
	$(log_footer)

lint-makefile: ## Check Makefile conventions
	@bash scripts/lint/check-makefile.sh
	$(log_footer)

lint-scripts: ## Check shell script conventions (shellcheck)
	@bash scripts/lint/check-scripts.sh
	$(log_footer)

lint-versions: ## Check version pinning (no hardcoded versions)
	@bash scripts/lint/check-version-pins.sh
	$(log_footer)

tidy: ## Run clang-tidy (smart: changed files only)
	@bash scripts/lint/run-tidy.sh $(if $(filter 1,$(FULL)),--full)
	$(log_footer)

complexity: ## Check cyclomatic complexity (pmccabe)
	@bash scripts/lint/check-complexity.sh
	$(log_footer)

comment-ratio: ## Show comment ratio per file
	@bash scripts/lint/check-comment-ratio.sh
	$(log_footer)

consistency: ## Check code consistency (ADR-065)
	@bash scripts/lint/check-consistency.sh
	$(log_footer)

check-theme: ## Check no hardcoded ANSI outside tui/ (ADR-080)
	@bash scripts/lint/check-theme.sh
	$(log_footer)

check-xref: ## Validate ADR cross-references in code (ADR-022)
	@bash scripts/lint/check-xref.sh
	$(log_footer)

check-interactive-input: ## Check no direct std::cin usage (ADR-088)
	@bash scripts/lint/check-interactive-input.sh
	$(log_footer)

dead-code: ## Detect unused functions and orphaned scripts (ADR-064)
	@bash scripts/lint/check-dead-code.sh
	$(log_footer)

dead-docs: ## Detect unreferenced docs, configs, and backlog items
	@bash scripts/lint/check-dead-docs.sh
	$(log_footer)

duplication: ## Detect duplicated code blocks (CPD or line-hash fallback)
	@bash scripts/lint/check-duplication.sh
	$(log_footer)

feature-density: ## Check LOG_FEATURE marker density (ADR-063)
	@bash scripts/test/check-feature-density.sh
	$(log_footer)

docs: ## Check doxygen warnings
	@echo "==> checking doxygen..."
	@output=$$(doxygen .config/Doxyfile 2>&1) || { echo "doxygen failed"; exit 1; }; \
	echo "$$output" | grep "warning:" | grep -v "No output formats\|Unsupported xml\|falsely parses" && exit 1 || true
	@echo "  [done] docs"

file-size: ## Check source file sizes (ADR-061)
	@bash scripts/lint/check-file-size.sh
	$(log_footer)

##@ Testing

test-unit: ## Run unit tests
	@bash scripts/test/run-unit.sh "$(BUILD_DIR)"
	$(log_footer)
t: test-unit

e2e: ## Run end-to-end tests
	@bash scripts/test/run-e2e.sh "$(BUILD_DIR)" "$(BINARY)"
	$(log_footer)

live: ## Integration test with real LLM
	@bash e2e/test_live.sh $(BINARY)

bench: ## Benchmark local Ollama models (see docs/model-bench.md)
	@bash scripts/test/bench-models.sh $(ARGS)
	$(log_footer)

preflight: ## Quick model capability check (math, reasoning, speed)
	@bash scripts/test/preflight.sh $(ARGS)
	$(log_footer)

coverage: ## Build with coverage and run tests
	@bash scripts/test/run-coverage.sh "$(BUILD_DIR)"
	$(log_footer)

coverage-report: coverage ## Show coverage summary per directory
	@bash scripts/test/report-coverage.sh "$(BUILD_DIR)"
	$(log_footer)

quick: ## Fast feedback: build + unit tests + comment ratio
	@$(MAKE) build
	@bash scripts/dev/quick.sh "$(BUILD_DIR)"
	$(log_footer)

features: ## List all test scenarios (feature spec)
	@for bin in test_repl test_annotations test_annotation test_config test_command test_json test_exec test_markdown test_logger test_trace test_scan test_hardware test_ollama; do \
		[ -f "$(BUILD_DIR)/$$bin" ] && ./$(BUILD_DIR)/$$bin --list-test-cases 2>/dev/null | grep "Scenario:" | sed 's/^    //'; \
	done

fuzz: ## Run annotation fuzzer (60s, requires llvm)
	@LLVM=$$(brew --prefix llvm 2>/dev/null || echo /usr/bin); \
	cmake -B build-fuzz -DENABLE_FUZZ=ON -DCMAKE_C_COMPILER="$$LLVM/bin/clang" -DCMAKE_CXX_COMPILER="$$LLVM/bin/clang++" > /dev/null && \
	cmake --build build-fuzz --target fuzz_annotation > /dev/null
	@echo "==> fuzzing annotation parser (60s)..."
	@./build-fuzz/fuzz_annotation -max_total_time=60 -print_final_stats=1

##@ Security

sast: sast-security sast-stegano sast-iac sast-secret sast-trufflehog sast-grype sast-osv sast-checkov ## Run all SAST checks

sast-security: ## Run semgrep security scan
	@echo "==> running sast-security (semgrep)..."
	@if command -v semgrep >/dev/null; then \
		semgrep scan --config auto --error --quiet 2>&1 | grep -v "┌────\|Semgrep CLI\|└─────────────" || true; \
	else echo "  [skip] semgrep not installed"; fi

sast-stegano: ## Run steganography scan (zsteg)
	@bash scripts/security/steg-check.sh
	$(log_footer)

sast-iac: ## Run IaC security scan (trivy)
	@if command -v trivy >/dev/null; then \
		trivy fs --config .config/trivy.yaml --scanners misconfig --severity HIGH,CRITICAL . ; \
	else echo "  [skip] trivy not installed"; fi

sast-secret: ## Run gitleaks secret scan
	@echo "==> running sast-secret (gitleaks...)"
	@if command -v gitleaks >/dev/null; then \
		gitleaks detect --source . --log-level error --no-banner; \
	else echo "  [skip] gitleaks not installed"; fi
	@echo "  [done] sast-secret"

sast-trufflehog: ## Run trufflehog verified secret scan
	@bash scripts/security/trufflehog-scan.sh
	$(log_footer)

sast-grype: ## Run grype vulnerability scan
	@bash scripts/security/grype-scan.sh
	$(log_footer)

sast-osv: ## Run osv-scanner vulnerability scan
	@bash scripts/security/osv-scan.sh
	$(log_footer)

sast-checkov: ## Run checkov IaC policy scan
	@bash scripts/security/checkov-scan.sh
	$(log_footer)

sast-codeql: ## Run CodeQL deep analysis (slow)
	@bash scripts/security/codeql-scan.sh
	$(log_footer)

sbom: ## Generate SBOM with syft
	@bash scripts/security/syft-sbom.sh
	$(log_footer)

summarize-safe: ## Summarize headers (skips if Ollama unavailable)
	@if curl -s --max-time 3 "http://$${OLLAMA_HOST:-localhost}:$${OLLAMA_PORT:-11434}/api/tags" >/dev/null 2>&1; then \
		bash scripts/dev/summarize-headers.sh; \
	else \
		echo "  [skip] summarize — Ollama not reachable"; \
	fi

sonar: ## Run SonarCloud scan (requires SONAR_TOKEN)
	@bash scripts/security/sonar-scan.sh
	$(log_footer)

sonar-report: ## Show SonarCloud issue summary (ARGS=BLOCKER for detail)
	@bash scripts/security/sonar-report.sh $(ARGS)
	$(log_footer)

##@ Development

learn: ## AI-assisted pattern discovery (5-min budget, uses llama-cli)
	@bash scripts/dev/learn.sh $(ARGS)
	$(log_footer)

update: ## Update all development tools to pinned versions
	@bash scripts/dev/update-tools.sh
	$(log_footer)

bump: ## Bump version (make bump PART=patch|minor|major)
	@bash scripts/dev/bump.sh "$(or $(PART),$(filter major minor patch,$(MAKECMDGOALS)))"
	$(log_footer)

trivi: ## Run implicit decision scan
	@bash scripts/dev/trivi.sh
	$(log_footer)

major minor patch:
	@true

log: ## View event logs
	@bash scripts/dev/log-viewer.sh $(ARGS)
	$(log_footer)

todo: ## Show TODO items from docs and code
	@bash scripts/dev/todo.sh
	$(log_footer)

index: ## Regenerate INDEX.md
	@bash scripts/dev/build-index.sh
	$(log_footer)

summarize: ## Summarize file headers with Ollama (--dry-run supported)
	@bash scripts/dev/summarize-headers.sh $(ARGS)
	$(log_footer)

precommit: ## Run pre-commit checks
	@bash scripts/git/precommit-check.sh
	$(log_footer)

prepush: ## Run pre-push checks
	@bash scripts/git/prepush-check.sh
	$(log_footer)

##@ GitHub

gh-pipeline-status: ## Show latest pipeline status (alias: gpls)
	@bash scripts/gh/pipeline-status.sh 2>&1 | tee .tmp/$@.log
	$(log_footer)
gpls: gh-pipeline-status

gh-pr-status: ## Show failed PR jobs (alias: gps)
	@bash scripts/gh/pr-status.sh $(ARGS) 2>&1 | tee .tmp/$@.log
	$(log_footer)
gps: gh-pr-status

gh-create-pr: ## Create pull request (alias: gpr)
	@bash scripts/gh/create-pr.sh 2>&1 | tee .tmp/$@.log
	$(log_footer)
gpr: gh-create-pr

gh-download-issues: ## Download GitHub issues (alias: gdi)
	@bash scripts/gh/download-issues.sh
	$(log_footer)
gdi: gh-download-issues

gh-pr-feedback: ## Show CodeRabbit feedback (alias: gpf)
	@bash scripts/gh/pr-feedback.sh
	$(log_footer)
gpf: gh-pr-feedback

gh-pr-resolve: ## Resolve CodeRabbit threads (alias: gpr-resolve)
	@bash scripts/gh/pr-resolve.sh $(ARGS)
	$(log_footer)

create-issue: ## Create issue (TITLE="..." DESC="...")
	@bash scripts/gh/create-issue.sh "$(TITLE)" "$(DESC)"
	$(log_footer)

##@ Help

help: ## Show this help
	@awk 'BEGIN {FS = ":.*##"; printf "Usage:\n  make \033[36m<target>\033[0m\n"} \
		/^[a-zA-Z_0-9-]+:.*?##/ { printf "  \033[36m%-25s\033[0m %s\n", $$1, $$2 } \
		/^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) }' $(MAKEFILE_LIST)

# Internal targets
all: $(if $(filter 1,$(SKIP_DEPS)),,check-deps)
	@cmake -B $(BUILD_DIR) -S . > /dev/null
	@cmake --build $(BUILD_DIR) --target llama-cli > /dev/null
	@if [ -f llama-cli ] && lsof llama-cli >/dev/null 2>&1; then \
		mv llama-cli llama-cli.old_$$(date +%s) 2>/dev/null || true; \
	fi
	@cp $(BINARY) .

check-deps:
	@bash scripts/lint/check-deps.sh
	$(log_footer)

check-versions:
	@bash scripts/lint/check-versions.sh
	$(log_footer)

# Catch-all: suggest similar targets for typos
%:
	@echo "Unknown target: $@"
	@echo "Did you mean one of these?"
	@awk '/^[a-zA-Z_0-9-]+:.*?##/ { printf "  %s\n", $$1 }' $(MAKEFILE_LIST) | grep -i "$(shell echo $@ | cut -c1-4)" || \
		echo "  make help    (show all targets)"
