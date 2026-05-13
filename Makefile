BUILD_DIR = build
BINARY = $(BUILD_DIR)/llama-cli

# FULL=1 disables smart mode for exhaustive checks (e.g. clang-tidy)
FULL ?= 0

# LOG=1 enables automatic tee to .tmp/<target>.log for any target
LOG ?= 1
ifdef LOG
ifneq ($(LOG),0)
# Wrap recipe output: tee to .tmp/<target>.log and print path at end
SHELL := /bin/bash
.SHELLFLAGS := -o pipefail -c
export MAKE_LOG_DIR := .tmp
$(shell mkdir -p .tmp)
endif
endif

.DEFAULT_GOAL := help

.PHONY: all build clean run start s log test t test-unit e2e record-e2e check check-fast check-all full-check mutation check-ai \
	format format-code format-yaml format-md format-scripts \
	lint lint-code lint-yaml lint-md lint-makefile lint-scripts lint-versions \
	tidy complexity comment-ratio docs file-size sast sast-secret sast-security sast-stegano sast-iac sast-trufflehog sast-grype sast-osv sast-checkov sast-codeql sbom consistency \
	coverage coverage-report todo quick index setup install hooks bench features fuzz update \
	sonar sonar-report trivi learn \
	live-full log-analysis ci-analysis \
	gh-pipeline-status gpls gh-pr-status gps gh-create-pr gpr \
	gh-download-issues gdi gh-pr-feedback gpf create-issue \
	bump major minor patch \
	pipeline-coverage check-unicode check-conversions check-casts check-portability check-shadowing check-traceability

##@ Getting Started

setup: ## Install all dependencies
	@bash lib/cpm/shell/run.sh setup bash scripts/dev/setup.sh

build: all ## Build the project
#      	   shellcheck           ✓
#          yamllint             ✓
#          rumdl                ✓
	@if [ "$${CI:-}" = "true" ]; then \
		echo "  build                ✓ v$$(git describe --tags --abbrev=0 2>/dev/null | sed 's/^v//' || echo '0.0.0') ($$(git rev-parse --short HEAD))"; \
	else \
		dirty=""; git diff --quiet -- 'src/*.cpp' 'src/*.h' 'src/**/*.cpp' 'src/**/*.h' 2>/dev/null || dirty=" dirty"; \
		echo "  build                ✓ $$(git rev-parse --short HEAD)$$dirty"; \
	fi

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

build-gcc: ## Cross-compile with GCC (catches CI-only errors, ~6 min)
	@cmake -B build-gcc -S . -DCMAKE_CXX_COMPILER=g++ > /dev/null
	@cmake --build build-gcc > /dev/null
	@echo "  build-gcc            ✓"

clean: ## Remove build artifacts
	rm -rf $(BUILD_DIR) llama-cli

kill: ## Kill running llama-cli instances
	@pkill -xf '(\./)?llama-cli|build/llama-cli' 2>/dev/null && echo "  killed llama-cli" || echo "  no llama-cli running"

##@ Aggregators

format: format-code format-md format-yaml format-scripts ## Auto-format all files

lint: lint-code lint-md lint-yaml lint-makefile lint-scripts lint-versions tidy complexity comment-ratio docs file-size consistency check-theme check-xref check-interactive-input check-pii slop check-unicode check-portability research-freshness ## Run all passive checks

test: build test-unit e2e ## Run all tests (builds first)

check-fast: build format ## Tier 1: format + build (AI auto-fix loop)
	@mkdir -p .tmp
	@$(MAKE) build format 2>&1 | tee .tmp/check-fast.log

check: ## Tier 2: full quality gate (CI/pre-push)
	@mkdir -p .tmp
	@$(MAKE) build lint test sast 2>&1 | tee .tmp/check.log

check-all: ## Tier 3: everything — exhaustive lint, mutation, SBOM, docs (alias: full-check)
	@mkdir -p .tmp
	@$(MAKE) FULL=1 build lint test sast mutation sbom todo summarize-safe index dead-code dead-docs duplication check-casts check-conversions check-shadowing check-traceability pipeline-coverage 2>&1 | tee .tmp/check-all.log
full-check: check-all

mutation: ## Run mutation testing (Mull, slow — PR only)
	@bash lib/cpm/shell/run.sh run-mutation bash scripts/test/run-mutation.sh

check-ai: ## Run checks with condensed output
	@$(MAKE) -s check 2>&1 | grep -E "^\s*([0-9]+|src/|==>|\[|FAIL|All|knownCondition|always false|too many|warning:|error:)" | grep -v "^$$"

##@ Formatting

format-code: ## Format C++ code (clang-format)
	@bash lib/cpm/shell/run.sh format-code bash scripts/fmt/format-code.sh

format-md: ## Format Markdown files (rumdl)
	@bash lib/cpm/shell/run.sh format-md bash scripts/fmt/format-md.sh

format-yaml: ## Format YAML files (trailing whitespace)
	@bash lib/cpm/shell/run.sh format-yaml bash scripts/fmt/format-yaml.sh

format-scripts: ## Format shell scripts (shfmt)
	@bash lib/cpm/shell/run.sh format-scripts bash scripts/fmt/format-scripts.sh

##@ Linting

lint-code: lint-format-code lint-cppcheck ## Lint C++ code (format + cppcheck)

lint-format-code: ## Check C++ formatting (no changes)
	@echo "==> checking C++ formatting..."
	@find src -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run -Werror --style=file:.config/.clang-format
	@echo "  [done] lint-format-code"

lint-cppcheck: ## Run cppcheck static analysis
	@bash lib/cpm/shell/run.sh lint-code bash scripts/lint/lint-code.sh

lint-md: ## Lint Markdown files (rumdl)
	@bash lib/cpm/shell/run.sh lint-md bash scripts/lint/lint-md.sh

lint-yaml: ## Lint YAML files (yamllint)
	@bash lib/cpm/shell/run.sh lint-yaml bash scripts/lint/lint-yaml.sh

lint-makefile: ## Check Makefile conventions
	@bash lib/cpm/shell/run.sh check-makefile bash scripts/lint/check-makefile.sh

lint-scripts: ## Check shell script conventions (shellcheck)
	@bash lib/cpm/shell/run.sh check-scripts bash scripts/lint/check-scripts.sh

lint-versions: ## Check version pinning (no hardcoded versions)
	@bash lib/cpm/shell/run.sh check-version-pins bash scripts/lint/check-version-pins.sh

tidy: ## Run clang-tidy (smart: changed files only)
	@bash lib/cpm/shell/run.sh run-tidy bash scripts/lint/run-tidy.sh $(if $(filter 1,$(FULL)),--full)

complexity: ## Check cyclomatic complexity (pmccabe)
	@bash lib/cpm/shell/run.sh check-complexity bash scripts/lint/check-complexity.sh

comment-ratio: ## Show comment ratio per file
	@bash lib/cpm/shell/run.sh check-comment-ratio bash scripts/lint/check-comment-ratio.sh

consistency: ## Check code consistency (ADR-065)
	@bash lib/cpm/shell/run.sh check-consistency bash scripts/lint/check-consistency.sh

check-theme: ## Check no hardcoded ANSI outside tui/ (ADR-080)
	@bash lib/cpm/shell/run.sh check-theme bash scripts/lint/check-theme.sh

check-xref: ## Validate ADR cross-references in code (ADR-022)
	@bash lib/cpm/shell/run.sh check-xref bash scripts/lint/check-xref.sh

check-interactive-input: ## Check no direct std::cin usage (ADR-088)
	@bash lib/cpm/shell/run.sh check-interactive-input bash scripts/lint/check-interactive-input.sh

check-pii: ## Check for PII in source code (ADR-098)
	@bash lib/cpm/shell/run.sh check-pii bash scripts/lint/check-pii.sh

dead-code: ## Detect unused functions and orphaned scripts (ADR-064)
	@bash lib/cpm/shell/run.sh check-dead-code bash scripts/lint/check-dead-code.sh

dead-docs: ## Detect unreferenced docs, configs, and backlog items
	@bash lib/cpm/shell/run.sh check-dead-docs bash scripts/lint/check-dead-docs.sh

duplication: ## Detect duplicated code blocks (CPD or line-hash fallback)
	@bash lib/cpm/shell/run.sh check-duplication bash scripts/lint/check-duplication.sh

slop: ## Detect AI-generated code slop patterns
	@bash lib/cpm/shell/run.sh check-slop bash scripts/lint/check-slop.sh

check-unicode: ## Check for invisible Unicode backdoors (ADR-097)
	@bash lib/cpm/shell/run.sh check-unicode bash scripts/lint/check-unicode.sh

check-portability: ## Detect cross-platform issues (ADR-093)
	@bash lib/cpm/shell/run.sh check-portability bash scripts/lint/check-portability.sh

research-freshness: ## Warn when research-backed scripts are stale (ADR-120)
	@bash lib/cpm/shell/run.sh check-research-freshness bash scripts/lint/check-research-freshness.sh

research-update: ## Mark a research topic as freshly researched (TOPIC=...)
	@if [ -z "$(TOPIC)" ]; then echo "Usage: make research-update TOPIC=SLOP_DETECTION"; exit 1; fi
	@sed -i '' "s/^$(TOPIC)=.*/$(TOPIC)=$$(date +%Y-%m-%d)/" .config/research-dates.env
	@echo "  ✓ $(TOPIC) updated to $$(date +%Y-%m-%d)"

check-casts: ## Detect C-style casts (ES.48, slow — compiles)
	@bash lib/cpm/shell/run.sh check-casts bash scripts/lint/check-casts.sh

check-conversions: ## Detect implicit conversions (slow — compiles)
	@bash lib/cpm/shell/run.sh check-conversions bash scripts/lint/check-conversions.sh

check-shadowing: ## Detect variable shadowing (ES.12, slow — compiles)
	@bash lib/cpm/shell/run.sh check-shadowing bash scripts/lint/check-shadowing.sh

check-traceability: ## Bidirectional traceability check (ADR-095)
	@bash lib/cpm/shell/run.sh check-traceability bash scripts/ci/check-traceability.sh

pipeline-coverage: ## Verify all make targets are in CI or denylist
	@bash lib/cpm/shell/run.sh check-pipeline-coverage bash scripts/lint/check-pipeline-coverage.sh

cmmi: ## CMMI maturity level audit (ADR-048)
	@bash lib/cpm/shell/run.sh check-cmmi bash scripts/lint/check-cmmi.sh

smells: ## Detect engineering anti-patterns (fun but real)
	@bash lib/cpm/shell/run.sh check-smells bash scripts/lint/check-smells.sh

inclusivity: ## Inclusivity & accessibility lint (C4I)
	@bash lib/cpm/shell/run.sh check-inclusivity bash scripts/lint/check-inclusivity.sh

licenses: ## Check dependency licenses (permissive only)
	@bash lib/cpm/shell/run.sh check-licenses bash scripts/lint/check-licenses.sh

feature-density: ## Check LOG_FEATURE marker density (ADR-063)
	@bash lib/cpm/shell/run.sh check-feature-density bash scripts/test/check-feature-density.sh

docs: ## Check doxygen warnings
	@echo "==> checking doxygen..."
	@output=$$(doxygen .config/Doxyfile 2>&1) || { echo "doxygen failed"; exit 1; }; \
	echo "$$output" | grep "warning:" | grep -v "No output formats\|Unsupported xml\|falsely parses" && exit 1 || true
	@echo "  [done] docs"

file-size: ## Check source file sizes (ADR-061)
	@bash lib/cpm/shell/run.sh check-file-size bash scripts/lint/check-file-size.sh

##@ Testing

test-unit: ## Run unit tests
	@bash lib/cpm/shell/run.sh run-unit bash scripts/test/run-unit.sh "$(BUILD_DIR)"
t: test-unit

e2e: ## Run end-to-end tests
	@bash scripts/test/run-e2e.sh "$(BUILD_DIR)" "$(BINARY)"

record-e2e: build ## Record e2e tests as gifs (auto-discovers new tests)
	@bash scripts/dev/record-e2e.sh $(ARGS)

live: ## Integration test with real LLM
	@bash e2e/test_live.sh $(BINARY)

live-full: ## Full feature test with real LLM (~30s)
	@bash e2e/test_full_feature.sh $(BINARY)

log-analysis: ## Analyze llama-cli event logs (timing, features, errors)
	@bash lib/cpm/shell/run.sh log-analysis bash scripts/dev/log-analysis.sh $(ARGS)

ci-analysis: ## Analyze CI pipeline (timing, failures, success rate)
	@bash lib/cpm/shell/run.sh ci-analysis bash scripts/gh/ci-analysis.sh $(ARGS)

bench: ## Benchmark local Ollama models (see docs/model-bench.md)
	@bash lib/cpm/shell/run.sh bench-models bash scripts/test/bench-models.sh $(ARGS)

preflight: ## Quick model capability check (math, reasoning, speed)
	@bash lib/cpm/shell/run.sh preflight bash scripts/test/preflight.sh $(ARGS)

coverage: ## Build with coverage and run tests
	@bash lib/cpm/shell/run.sh run-coverage bash scripts/test/run-coverage.sh "$(BUILD_DIR)"

coverage-report: coverage ## Show coverage summary per directory
	@bash lib/cpm/shell/run.sh report-coverage bash scripts/test/report-coverage.sh "$(BUILD_DIR)"

quick: ## Fast feedback: build + unit tests + comment ratio
	@$(MAKE) build
	@bash lib/cpm/shell/run.sh quick bash scripts/dev/quick.sh "$(BUILD_DIR)"

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
	@bash lib/cpm/shell/run.sh steg-check bash scripts/security/steg-check.sh

sast-iac: ## Run IaC security scan (trivy)
	@if command -v trivy >/dev/null; then \
		trivy fs --config .config/trivy.yaml --scanners misconfig --severity HIGH,CRITICAL . ; \
	else echo "  [skip] trivy not installed"; fi

sast-secret: ## Run gitleaks secret scan
	@echo "==> running sast-secret (gitleaks...)"
	@bash lib/cpm/shell/run.sh sast-secret bash scripts/security/sast-secret.sh 2>&1 | tee .tmp/sast-secret.log
	@echo "  [done] sast-secret"

sast-trufflehog: ## Run trufflehog verified secret scan
	@bash lib/cpm/shell/run.sh trufflehog-scan bash scripts/security/trufflehog-scan.sh

sast-grype: ## Run grype vulnerability scan
	@bash lib/cpm/shell/run.sh grype-scan bash scripts/security/grype-scan.sh

sast-osv: ## Run osv-scanner vulnerability scan
	@bash lib/cpm/shell/run.sh osv-scan bash scripts/security/osv-scan.sh

sast-checkov: ## Run checkov IaC policy scan
	@bash lib/cpm/shell/run.sh checkov-scan bash scripts/security/checkov-scan.sh

sast-codeql: ## Run CodeQL deep analysis (slow)
	@bash lib/cpm/shell/run.sh codeql-scan bash scripts/security/codeql-scan.sh

sbom: ## Generate SBOM with syft
	@bash lib/cpm/shell/run.sh syft-sbom bash scripts/security/syft-sbom.sh

summarize-safe: ## Summarize headers (skips if Ollama unavailable)
	@if curl -s --max-time 3 "http://$${OLLAMA_HOST:-localhost}:$${OLLAMA_PORT:-11434}/api/tags" >/dev/null 2>&1; then \
		bash scripts/dev/summarize-headers.sh 2>&1 | tee .tmp/summarize-safe.log; \
	else \
		echo "  [skip] summarize — Ollama not reachable"; \
	fi

sonar: ## Run SonarCloud scan (requires SONAR_TOKEN)
	@bash lib/cpm/shell/run.sh sonar-scan bash scripts/security/sonar-scan.sh

sonar-report: ## Show SonarCloud issue summary (ARGS=BLOCKER for detail)
	@bash lib/cpm/shell/run.sh sonar-report bash scripts/security/sonar-report.sh $(ARGS)

##@ Development

learn: ## AI-assisted pattern discovery (5-min budget, uses llama-cli)
	@bash lib/cpm/shell/run.sh learn bash scripts/dev/learn.sh $(ARGS)

update: ## Update all development tools to pinned versions
	@bash lib/cpm/shell/run.sh update-tools bash scripts/dev/update-tools.sh

bump: ## Bump version (make bump PART=patch|minor|major)
	@bash lib/cpm/shell/run.sh bump bash scripts/dev/bump.sh "$(or $(PART),$(filter major minor patch,$(MAKECMDGOALS)))"

trivi: ## Run implicit decision scan
	@bash lib/cpm/shell/run.sh trivi bash scripts/dev/trivi.sh

new: ## Scaffold new file (make new TYPE=cpp NAME=module/file BRIEF="...")
	@bash lib/cpm/shell/run.sh scaffold bash scripts/dev/scaffold.sh $(ARGS) TYPE=$(TYPE) NAME=$(NAME) BRIEF="$(BRIEF)"

major minor patch:
	@true

log: ## View event logs
	@bash lib/cpm/shell/run.sh log-viewer bash scripts/dev/log-viewer.sh $(ARGS)

commit-stats: ## Show commit type/scope distribution and health metrics
	@bash lib/cpm/shell/run.sh commit-stats bash scripts/dev/commit-stats.sh $(ARGS)

todo: ## Show TODO items from docs and code
	@bash lib/cpm/shell/run.sh todo bash scripts/dev/todo.sh

index: ## Regenerate INDEX.md
	@bash lib/cpm/shell/run.sh build-index bash scripts/dev/build-index.sh

summarize: ## Summarize file headers with Ollama (--dry-run supported)
	@bash lib/cpm/shell/run.sh summarize-headers bash scripts/dev/summarize-headers.sh $(ARGS) 2>&1 | tee .tmp/summarize.log

precommit: ## Run pre-commit checks
	@bash lib/cpm/shell/run.sh precommit-check bash scripts/git/precommit-check.sh

prepush: ## Run pre-push checks
	@bash lib/cpm/shell/run.sh prepush-check bash scripts/git/prepush-check.sh

pre-pr: ## Full pre-PR validation (build both compilers, lint, test)
	@echo "==> Pre-PR validation"
	@$(MAKE) build
	@$(MAKE) test-unit
	@$(MAKE) e2e
	@$(MAKE) lint
	@$(MAKE) pipeline-coverage
	@echo ""
	@echo "  ✓ Ready to open PR"

##@ GitHub

gh-pipeline-status: ## Show latest pipeline status (alias: gpls)
	@bash lib/cpm/shell/run.sh pipeline-status bash scripts/gh/pipeline-status.sh 2>&1 | tee .tmp/$@.log
gpls: gh-pipeline-status

gh-pr-status: ## Show failed PR jobs (alias: gps)
	@bash lib/cpm/shell/run.sh pr-status bash scripts/gh/pr-status.sh $(ARGS) 2>&1 | tee .tmp/$@.log
gps: gh-pr-status

gh-pr-check: ## Full PR review: CI + SonarCloud + CodeRabbit (alias: gpc)
	@(echo "── CI Pipeline ──" && \
	bash scripts/gh/pr-status.sh 2>&1 | tail -n +3 && \
	echo "" && \
	echo "── SonarCloud ──" && \
	bash scripts/gh/pr-sonar.sh 2>&1 | tail -n +3 && \
	echo "" && \
	echo "── CodeRabbit ──" && \
	bash scripts/gh/pr-feedback.sh 2>&1 | tail -n +1) | tee .tmp/gh-pr-check.log
gpc: gh-pr-check

gh-create-pr: ## Create draft pull request (alias: gpr)
	@bash lib/cpm/shell/run.sh create-pr bash scripts/gh/create-pr.sh 2>&1 | tee .tmp/$@.log
gpr: gh-create-pr

gh-pr-ready: ## Mark PR ready for review (triggers heavy checks)
	@gh pr ready
	@echo "  ✓ PR marked ready — coverage + sanitizers + macOS build will run"
gprr: gh-pr-ready

gh-download-issues: ## Download GitHub issues (alias: gdi)
	@bash lib/cpm/shell/run.sh download-issues bash scripts/gh/download-issues.sh
gdi: gh-download-issues

gh-pr-feedback: ## Show CodeRabbit feedback (alias: gpf)
	@bash lib/cpm/shell/run.sh pr-feedback bash scripts/gh/pr-feedback.sh
gpf: gh-pr-feedback

gh-pr-resolve: ## Resolve CodeRabbit threads (alias: gpr-resolve)
	@bash lib/cpm/shell/run.sh pr-resolve bash scripts/gh/pr-resolve.sh $(ARGS)

create-issue: ## Create issue (TITLE="..." DESC="...")
	@bash lib/cpm/shell/run.sh create-issue bash scripts/gh/create-issue.sh "$(TITLE)" "$(DESC)"

release: ## Trigger a GitHub release (from main branch)
	@bash lib/cpm/shell/run.sh release bash scripts/gh/release.sh $(ARGS)

merge: ## Merge current PR (squash)
	@gh pr merge --squash --delete-branch
	@echo "  ✓ PR merged and branch deleted"

gh-cleanup: ## Remove merged branches (dry-run; use ARGS=--apply to delete)
	@bash lib/cpm/shell/run.sh gh-cleanup bash scripts/gh/gh-cleanup.sh $(ARGS)

##@ Help

help: ## Show this help
	@awk 'BEGIN {FS = ":.*##"; printf "Usage:\n  make \033[36m<target>\033[0m\n"} \
		/^[a-zA-Z_0-9-]+:.*?##/ { printf "  \033[36m%-25s\033[0m %s\n", $$1, $$2 } \
		/^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) }' $(MAKEFILE_LIST)

# Internal targets
all: $(if $(filter 1,$(SKIP_DEPS)),,check-deps)
	@cmake -B $(BUILD_DIR) -S . > /dev/null
	@cmake --build $(BUILD_DIR) --target llama-cli > /dev/null
	@mv -f llama-cli llama-cli.old 2>/dev/null || true
	@cp $(BINARY) .

check-deps:
	@bash lib/cpm/shell/run.sh check-deps bash scripts/lint/check-deps.sh

check-versions:
	@bash lib/cpm/shell/run.sh check-versions bash scripts/lint/check-versions.sh

# Catch-all: suggest similar targets for typos
%:
	@echo "Unknown target: $@"
	@echo "Did you mean one of these?"
	@awk '/^[a-zA-Z_0-9-]+:.*?##/ { printf "  %s\n", $$1 }' $(MAKEFILE_LIST) | grep -i "$(shell echo $@ | cut -c1-4)" || \
		echo "  make help    (show all targets)"

# CPM integration (Phase 1: shell lib only, make targets later)
# @see docs/adr/adr-121-cpm-quality-layer.md
# Scripts source lib/cpm/shell/ui.sh with fallback if symlink absent
