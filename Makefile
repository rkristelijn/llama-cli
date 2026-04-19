BUILD_DIR = build
CLANG_TIDY = $(shell command -v clang-tidy 2>/dev/null || echo /opt/homebrew/opt/llvm/bin/clang-tidy)

# SMART=1 (default) enables incremental checks (only changed files vs main)
# FULL=1 disables smart mode for exhaustive checks (used in full-check target)
FULL ?= 0
ifeq ($(FULL),1)
  SMART := 0
else
  SMART ?= 1
endif

.DEFAULT_GOAL := help

.PHONY: all build clean run start s log test t test-unit test-e2e end-to-end e2e check full-check check-ai format cpp-format install hooks help quick index comment-ratio check-deps check-versions live tidy lint yamllint markdownlint lint-makefile lint-scripts complexity docs sast sast-secret sast-security coverage coverage-folder todo create-issue gh-pr-status gps gh-pipeline-status gpls gh-create-pr gpr gh-pr-feedback gpf gh-download-issues gdi prepush bump major minor patch

##@ Getting Started

setup: ## Install all dependencies
	bash scripts/dev/setup.sh

build: all ## Build the project

start: all ## Build and run the REPL (alias: s)
	./$(BUILD_DIR)/llama-cli $(ARGS)
s: start

run: start ## Alias for start
r: run

install: all ## Install to /usr/local/bin
	@cp $(BUILD_DIR)/llama-cli /usr/local/bin/llama-cli
	@echo "Installed to /usr/local/bin/llama-cli"

hooks: ## Install git hooks (pre-commit, pre-push)
	@cp scripts/git/pre-commit.sh .git/hooks/pre-commit
	@cp scripts/git/pre-push.sh .git/hooks/pre-push
	@chmod +x .git/hooks/pre-commit .git/hooks/pre-push
	@echo "Git hooks installed (pre-commit, pre-push)."

clean: ## Remove build artifacts
	rm -rf $(BUILD_DIR) llama-cli

##@ Quality

check: ## Run smart quality checks (shift-left order)
	@bash scripts/check/run-all.sh

full-check: ## Run exhaustive quality checks (FULL=1)
	@$(MAKE) FULL=1 check

check-ai: ## Run checks with condensed output
	@$(MAKE) -s check 2>&1 | grep -E "^\s*([0-9]+|src/|==>|\[|FAIL|All|knownCondition|always false|too many|warning:|error:)" | grep -v "^$$"

check-deps: ## Verify required tools are installed
	@bash scripts/check/check-deps.sh

check-versions: ## Compare installed versions against versions.env
	@bash scripts/check/check-versions.sh

##@ Testing

test: test-unit ## Run unit tests (alias: t)
t: test

test-unit: all ## Build and run unit tests
	@bash scripts/check/test-unit.sh "$(BUILD_DIR)"

e2e: build ## Run end-to-end tests (alias: e2e)
	@echo "==> make e2e"
	@for t in e2e/*.sh; do case "$$t" in *test_live*|*helpers*) continue;; esac; bash "$$t" $(BUILD_DIR)/llama-cli > /dev/null || { echo "FAIL: $$t"; exit 1; }; done
	@echo "  [done] e2e"
end-to-end: e2e

live: all ## Integration test with real LLM
	@bash e2e/test_live.sh $(BUILD_DIR)/llama-cli

coverage: ## Build with coverage and run tests
	@bash scripts/check/run-coverage.sh "$(BUILD_DIR)"

coverage-folder: coverage ## Show coverage summary per directory
	@bash scripts/check/coverage-folder.sh "$(BUILD_DIR)"

quick: all ## Fast feedback: unit tests + comment ratio
	@bash scripts/dev/quick.sh "$(BUILD_DIR)"

##@ Linting

format: format-code format-yaml format-markdown format-scripts ## Auto-format all

format-code: ## Auto-format C++ source files
	@find src -name '*.cpp' -o -name '*.h' | xargs clang-format -i --style=file:.config/.clang-format

format-yaml: ## Auto-format YAML files
	@if command -v yamllint >/dev/null; then yamllint -d relaxed -f parsable .github/ | awk -F: '{print $$1}' | sort -u | while read -r f; do sed -i.bak 's/[[:space:]]*$$//' "$$f" && rm -f "$$f.bak"; done; fi

format-markdown: ## Auto-format Markdown files
	@if command -v rumdl >/dev/null; then rumdl fmt .; fi

format-scripts: ## Auto-format shell scripts
	@if command -v shfmt >/dev/null; then find scripts -name '*.sh' -exec shfmt -i 2 -w {} \;; fi

cpp-format: ## Check C++ formatting (no changes)
	@echo "==> make cpp-format"
	@find src -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run -Werror --style=file:.config/.clang-format
	@echo "  [done] cpp-format"

tidy: all ## Run clang-tidy (smart: changed files only)
	@bash scripts/check/tidy.sh $(if $(filter 1,$(FULL)),--full)

lint-cpp: all ## Run cppcheck static analysis
	@echo "==> make lint-cpp (running cppcheck...)"
	@output=$$(cppcheck --enable=all --suppress=missingIncludeSystem --suppress=missingInclude --suppress=unusedFunction --suppress=unmatchedSuppression --suppress=normalCheckLevelMaxBranches --suppress=checkersReport --suppress=useStlAlgorithm --suppress=knownConditionTrueFalse:*_it.cpp --suppress=knownConditionTrueFalse:*_test.cpp --error-exitcode=1 -I src/ src/ 2>&1) || { echo "$$output" | grep -v "Checking\|files checked"; exit 1; }; \
	echo "$$output" | grep -v "Checking\|files checked" || true
	@echo "  [done] lint-cpp"

lint-code: cpp-format lint-cpp ## Lint C++ code (format check + cppcheck)

complexity: all ## Check cyclomatic complexity (pmccabe)
	@bash scripts/check/complexity.sh

lint-yaml: ## Lint YAML files
	@echo "==> make lint-yaml"
	@if command -v yamllint >/dev/null; then yamllint -c .config/yamllint.yml .github/; \
	else echo "  [skip] yamllint not installed"; fi
	@echo "  [done] lint-yaml"

lint-markdown: ## Lint Markdown files (rumdl)
	@echo "==> make lint-markdown"
	@if command -v rumdl >/dev/null; then rumdl check .; \
	else echo "  [skip] rumdl not installed"; fi
	@echo "  [done] lint-markdown"

lint-makefile: ## Check Makefile targets are ≤5 lines
	@echo "==> make lint-makefile"
	@bash scripts/check/lint-makefile.sh
	@echo "  [done] lint-makefile"

lint-scripts: ## Check shell script conventions
	@echo "==> make lint-scripts"
	@bash scripts/check/lint-scripts.sh
	@echo "  [done] lint-scripts"

lint: lint-code lint-yaml lint-markdown lint-makefile lint-scripts ## Run all linting checks

docs: ## Check doxygen warnings
	@echo "==> make docs (generating doxygen...)"
	@output=$$(doxygen .config/Doxyfile 2>&1) || { echo "doxygen failed"; exit 1; }; \
	echo "$$output" | grep "warning:" | grep -v "No output formats\|Unsupported xml\|falsely parses" && exit 1 || true
	@echo "  [done] docs"

comment-ratio: ## Show comment ratio per file
	@cloc src/ --not-match-f='(_test|_it)\.cpp$$' --by-file --csv --quiet \
	  | grep -v "^language\|^SUM\|^http" \
	  | awk -F',' 'NF==5 && $$5>0 {ratio=int($$4/($$4+$$5)*100); printf "%d%%\t%s\n", ratio, $$2}' \
	  | sort -n

##@ Security

sast: sast-security sast-secret ## Run all SAST checks

sast-security: ## Run semgrep security scan
	@echo "==> make sast-security (semgrep...)"
	@if command -v semgrep >/dev/null; then \
		semgrep scan --config auto --error --quiet 2>&1 | grep -v "┌────\|Semgrep CLI\|└─────────────" || true; \
	else echo "  [skip] semgrep not installed"; fi
	@echo "  [done] sast-security"

sast-secret: ## Run gitleaks secret scan
	@echo "==> make sast-secret (gitleaks...)"
	@if command -v gitleaks >/dev/null; then \
		gitleaks detect --source . --log-level error --no-banner; \
	else echo "  [skip] gitleaks not installed"; fi
	@echo "  [done] sast-secret"

##@ Development

bump: ## Bump version (make bump patch|minor|major)
	@bash scripts/dev/bump.sh "$(or $(PART),$(filter major minor patch,$(MAKECMDGOALS)))"

major minor patch:
	@true

log: ## View event logs
	@bash scripts/dev/log-viewer.sh $(ARGS)

todo: ## Show TODO items from docs and code
	@bash scripts/dev/todo.sh

index: ## Regenerate INDEX.md
	@echo "==> make index"
	@bash scripts/dev/build-index.sh

precommit: ## Run pre-commit checks (format + build + secrets)
	@bash scripts/git/precommit-check.sh

prepush: ## Run pre-push checks (smart: code vs docs)
	@bash scripts/dev/prepush.sh

##@ GitHub

gh-pipeline-status: ## Show latest pipeline status (alias: gpls)
	bash scripts/gh/pipeline-status.sh
gpls: gh-pipeline-status

gh-pr-status: ## Show failed PR jobs (alias: gps)
	bash scripts/gh/pr-status.sh $(ARGS)
gps: gh-pr-status

gh-create-pr: ## Create pull request (alias: gpr)
	bash scripts/gh/create-pr.sh
gpr: gh-create-pr

gh-download-issues: ## Download GitHub issues (alias: gdi)
	bash scripts/gh/download-issues.sh
gdi: gh-download-issues

gh-pr-feedback: ## Show CodeRabbit feedback (alias: gpf)
	bash scripts/gh/pr-feedback.sh
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

# Internal targets (no ## comment = hidden from help)
all: $(if $(filter 1,$(SKIP_DEPS)),,check-deps)
	@cmake -B $(BUILD_DIR) -S . > /dev/null
	@cmake --build $(BUILD_DIR) --target llama-cli > /dev/null
	@cp $(BUILD_DIR)/llama-cli .
