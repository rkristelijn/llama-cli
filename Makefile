BUILD_DIR = build
CLANG_TIDY = $(shell command -v clang-tidy 2>/dev/null || echo /opt/homebrew/opt/llvm/bin/clang-tidy)

.PHONY: all clean run test check install help quick index comment-ratio pipeline-status pr-status download-issues

all:
	cmake -B $(BUILD_DIR) -S .
	cmake --build $(BUILD_DIR)

run: all
	./$(BUILD_DIR)/llama-cli

test: all
	cmake --build $(BUILD_DIR) --target test_config
	cmake --build $(BUILD_DIR) --target test_json
	cmake --build $(BUILD_DIR) --target test_repl
	cmake --build $(BUILD_DIR) --target test_command
	cmake --build $(BUILD_DIR) --target test_annotation
	cmake --build $(BUILD_DIR) --target test_exec
	cmake --build $(BUILD_DIR) --target test_conversation
	cmake --build $(BUILD_DIR) --target test_commands
	cmake --build $(BUILD_DIR) --target test_options
	cmake --build $(BUILD_DIR) --target test_annotations
	cmake --build $(BUILD_DIR) --target test_markdown
	./$(BUILD_DIR)/test_config
	./$(BUILD_DIR)/test_json
	./$(BUILD_DIR)/test_repl
	./$(BUILD_DIR)/test_command
	./$(BUILD_DIR)/test_annotation
	./$(BUILD_DIR)/test_exec
	./$(BUILD_DIR)/test_conversation
	./$(BUILD_DIR)/test_commands
	./$(BUILD_DIR)/test_options
	./$(BUILD_DIR)/test_annotations
	./$(BUILD_DIR)/test_markdown
	sh scripts/test_comment_ratio.sh

check: all test
	@echo "==> clang-tidy"
	@$(CLANG_TIDY) --config-file=.config/.clang-tidy src/*/*.cpp -- -std=c++17 -I src/ 2>&1 | grep "warning:" | grep -v "linenoise" && exit 1 || true
	@echo "==> pmccabe (complexity <= 10)"
	@pmccabe src/*/*.cpp | awk '$$1 > 10 {print; found=1} END {if (found) exit 1}'
	@echo "==> cppcheck"
	cppcheck --enable=all --suppress=missingIncludeSystem --suppress=missingInclude --suppress=unusedFunction --suppress=unmatchedSuppression --suppress=normalCheckLevelMaxBranches --suppress=checkersReport --suppress=useStlAlgorithm --error-exitcode=1 -I src/ src/
	@echo "==> doxygen lint"
	@doxygen .config/Doxyfile 2>&1 | grep "warning:" | grep -v "No output formats" && exit 1 || true
	@echo "==> index freshness"
	@sh scripts/build-index.sh > /dev/null && git diff --quiet INDEX.md || { echo "FAIL: INDEX.md is outdated. Run 'make index'"; exit 1; }
	@echo "==> coverage (>= 80%)"
	@sh scripts/test_coverage.sh
	@echo "==> semgrep"
	PATH="$$HOME/.local/bin:$$PATH" semgrep scan --config auto --error
	@echo "==> gitleaks"
	gitleaks detect --source .
	@echo "==> open items"
	@$(MAKE) -s todo
	@echo ""
	@echo "All checks passed."

# Install dependencies and git hooks
setup:
	sh scripts/setup.sh

install:
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
	sh scripts/pr-status.sh

# Download GitHub issues to .cache/issues/
download-issues:
	sh scripts/download-issues.sh

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
	rm -rf $(BUILD_DIR)

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
	@echo "  make run            build and run"
	@echo "  make quick          incremental build + tests (fast)"
	@echo "  make test           full build + tests"
	@echo "  make prepush        smart check (only what changed vs main)"
	@echo "  make check          run all quality checks"
	@echo "  make install        install git hooks"
	@echo "  make clean          remove build artifacts"
	@echo "  make index          regenerate INDEX.md"
	@echo "  make comment-ratio  show comment ratio per source file"
	@echo "  make pipeline-status show latest CI pipeline status"
	@echo "  make pr-status      show failed PR jobs"
	@echo "  make download-issues download GitHub issues to .cache/issues/"
