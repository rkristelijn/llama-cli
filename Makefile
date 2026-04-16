BUILD_DIR = build
CLANG_TIDY = $(shell command -v clang-tidy 2>/dev/null || echo /opt/homebrew/opt/llvm/bin/clang-tidy)

.PHONY: all build clean run start log test test-unit test-e2e e2e check check-ai format format-check install hooks help quick index comment-ratio pipeline-status pr-status pr pr-feedback download-issues check-deps

check-deps:
	@command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake not found. Run 'make setup' first."; exit 1; }

all: check-deps
	cmake -B $(BUILD_DIR) -S .
	cmake --build $(BUILD_DIR) --target llama-cli
	cp $(BUILD_DIR)/llama-cli .

build: all

run: all
	./$(BUILD_DIR)/llama-cli $(ARGS)

start: all
	./$(BUILD_DIR)/llama-cli $(ARGS)

log:
	@sh scripts/log-viewer.sh $(ARGS)

test-unit: all
	cmake --build $(BUILD_DIR) --target test_config
	cmake --build $(BUILD_DIR) --target test_json
	cmake --build $(BUILD_DIR) --target test_repl
	cmake --build $(BUILD_DIR) --target test_command
	cmake --build $(BUILD_DIR) --target test_annotation
	cmake --build $(BUILD_DIR) --target test_exec
	./$(BUILD_DIR)/test_config
	./$(BUILD_DIR)/test_json
	./$(BUILD_DIR)/test_repl
	./$(BUILD_DIR)/test_command
	./$(BUILD_DIR)/test_annotation
	./$(BUILD_DIR)/test_exec
	bash scripts/test_comment_ratio.sh

test: test-unit

e2e: test-e2e

test-e2e: build
	@echo "==> E2E tests"
	@for t in e2e/*.sh; do echo "Running $$t"; bash "$$t" $(BUILD_DIR)/llama-cli || exit 1; done
	@echo "E2E tests passed."

check: all test e2e format index
	@echo "==> clang-tidy"
	@# Suppress: identifier-naming (doctest), function-size (use NOLINT in source; logger filtered here)
	@$(CLANG_TIDY) --config-file=.config/.clang-tidy src/*/*.cpp -- -std=c++17 -I src/ 2>&1 | grep "warning:" | grep -v "linenoise\|SCENARIO\|cognitive complexity\|identifier-naming\|logging/logger.*function-size" && exit 1 || true
	@echo "==> pmccabe (complexity <= 10)"
	@find src -name '*.cpp' | xargs pmccabe 2>/dev/null | while read line; do \
		file=$$(echo $$line | awk '{print $$6}' | cut -d'(' -f1); \
		lno=$$(echo $$line | awk '{print $$6}' | cut -d'(' -f2 | cut -d')' -f1); \
		if ! head -n $$lno $$file | tail -n 6 | grep -q "pmccabe:skip-complexity\|clang-tidy:skip-complexity"; then \
			echo $$line | awk '$$1 > 10 {print; exit 1}'; \
		fi; \
	done || exit 1
	@echo "==> cppcheck"
	cppcheck --enable=all --suppress=missingIncludeSystem --suppress=missingInclude --suppress=unusedFunction --suppress=unmatchedSuppression --suppress=normalCheckLevelMaxBranches --suppress=checkersReport --suppress=useStlAlgorithm --suppress=knownConditionTrueFalse:*_it.cpp --suppress=knownConditionTrueFalse:*_test.cpp --error-exitcode=1 -I src/ src/
	@echo "==> doxygen lint"
	@doxygen .config/Doxyfile 2>&1 | grep "warning:" | grep -v "No output formats" && exit 1 || true
	@echo "==> index (auto-updated)"
	@echo "==> coverage (>= 80%)"
	@sh scripts/test_coverage.sh
	@echo "==> semgrep"
	@PATH="$$HOME/.local/bin:$$PATH" semgrep scan --config auto --error --quiet
	@echo "==> gitleaks"
	gitleaks detect --source .
	@echo "==> open items"
	@$(MAKE) -s todo
	@echo ""
	@echo "All checks passed."

# Like check, but minimal output for AI use. If output is noisy, update the grep filter in Makefile.
check-ai:
	@echo "[check-ai] AI-optimized output. Run 'make check' for full output. If noisy/incomplete, improve the grep filter in Makefile:check-ai"
	@$(MAKE) -s check 2>&1 | grep -E "^\s*([0-9]+|src/|==>|FAIL|All checks passed|knownCondition|always false|too many|warning:|error:)" | grep -v "^$$"


setup:
	sh scripts/setup.sh

install: all
	cp $(BUILD_DIR)/llama-cli /usr/local/bin/llama-cli
	@echo "Installed to /usr/local/bin/llama-cli"

hooks:
	cp .config/pre-commit .git/hooks/pre-commit
	chmod +x .git/hooks/pre-commit
	@echo "Git hooks installed."

todo:
	@echo "==> TODO.md"
	@grep -n "\- \[ \]" TODO.md 2>/dev/null || true
	@echo ""
	@echo "==> Code TODOs"
	@grep -rn "TODO\|FIXME\|HACK\|XXX" src/ include/ --include="*.cpp" --include="*.h" 2>/dev/null || true

# Generate INDEX.md from all project files
index:
	sh scripts/build-index.sh

# Apply clang-format to all source files
format:
	find src -name '*.cpp' -o -name '*.h' | xargs clang-format -i --style=file:.config/.clang-format

# Dry-run clang-format check (non-zero exit if violations)
format-check:
	find src -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run -Werror --style=file:.config/.clang-format
	@echo "format-check: OK"

# Show comment ratio per production file (excludes _test/_it)
comment-ratio:
	@cloc src/ --not-match-f='(_test|_it)\.cpp$$' --by-file --csv --quiet \
	  | grep -v "^language\|^SUM\|^http" \
	  | awk -F',' 'NF==5 && $$5>0 {ratio=int($$4/($$4+$$5)*100); printf "%d%%\t%s\n", ratio, $$2}' \
	  | sort -n

# Show latest pipeline status for current branch
pipeline-status:
	sh scripts/pipeline-status.sh

# Show failed PR jobs for current branch
pr-status:
	sh scripts/pr-status.sh $(ARGS)

# Create a pull request for current branch
pr:
	bash scripts/create-pr.sh

# Download GitHub issues to .cache/issues/
download-issues:
	bash scripts/download-issues.sh

# Download CodeRabbit feedback for a PR to .cache/pr/
pr-feedback:
	bash scripts/fetch-coderabbit-feedback.sh

# Create a new GitHub issue
create-issue:
	@if [ -z "$(TITLE)" ] || [ -z "$(DESC)" ]; then \
		echo "Usage: make create-issue TITLE=\"My title\" DESC=\"My description\""; \
		exit 1; \
	fi
	sh scripts/gh-create-issue.sh "$(TITLE)" "$(DESC)"

# Generate test coverage report
coverage:
	cmake -B $(BUILD_DIR) -S . -DCMAKE_CXX_FLAGS="--coverage"
	cmake --build $(BUILD_DIR)
	./$(BUILD_DIR)/test_config
	./$(BUILD_DIR)/test_json
	./$(BUILD_DIR)/test_repl
	@echo ""
	@echo "==> Coverage report"
	@gcov -n $(BUILD_DIR)/CMakeFiles/test_config.dir/src/config.cpp.o \
	         $(BUILD_DIR)/CMakeFiles/test_json.dir/src/json.cpp.o \
	         $(BUILD_DIR)/CMakeFiles/test_repl.dir/src/repl.cpp.o 2>&1 \
	  | grep -A1 "^File.*llama-cli/src\|^File.*llama-cli/include"

clean:
	rm -rf $(BUILD_DIR) llama-cli

# Quick: incremental build + run tests only (no static analysis)
quick: all
	@./$(BUILD_DIR)/test_config
	@./$(BUILD_DIR)/test_json
	@./$(BUILD_DIR)/test_repl
	@./$(BUILD_DIR)/test_command
	@./$(BUILD_DIR)/test_annotation
	@./$(BUILD_DIR)/test_exec
	@./$(BUILD_DIR)/test_conversation
	./$(BUILD_DIR)/test_commands
	./$(BUILD_DIR)/test_options
	./$(BUILD_DIR)/test_annotations
	./$(BUILD_DIR)/test_markdown
	@sh scripts/test_comment_ratio.sh

# Smart pre-push: only check what changed since main
prepush:
	@changed=$$(git diff --name-only origin/main...HEAD); \
	if echo "$$changed" | grep -qE '\.(cpp|h)$$'; then \
		echo "==> code changed: running full checks"; \
		$(MAKE) check; \
	else \
		echo "==> docs only: skipping code checks"; \
		$(MAKE) index; \
		git diff --quiet INDEX.md || { echo "FAIL: INDEX.md outdated"; exit 1; }; \
		echo "All checks passed."; \
	fi

help:
	@echo "Usage:"
	@echo "  make                build the project"
	@echo "  make run            build and run (alias: make start)"
	@echo "  make start          build and start REPL (ARGS=... for extra args)"
	@echo "  make quick          incremental build + tests (fast)"
	@echo "  make test           unit tests"
	@echo "  make e2e            end-to-end tests (requires Ollama)"
	@echo "  make prepush        smart check (only what changed vs main)"
	@echo "  make check          run all quality checks"
	@echo "  make install        build and install to /usr/local/bin"
	@echo "  make hooks          install git hooks"
	@echo "  make clean          remove build artifacts"
	@echo "  make index          regenerate INDEX.md"
	@echo "  make format         apply clang-format to all source files"
	@echo "  make format-check   dry-run clang-format (CI-style check)"
	@echo "  make pipeline-status show latest CI pipeline status"
	@echo "  make pr-status      show failed PR jobs"
	@echo "  make download-issues download GitHub issues to .cache/issues/"
	@echo "  make create-issue   create a new GitHub issue (TITLE=... DESC=...)"
