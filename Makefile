BUILD_DIR = build
CLANG_TIDY = $(shell command -v clang-tidy 2>/dev/null || echo /opt/homebrew/opt/llvm/bin/clang-tidy)

.PHONY: all clean run test check install help quick

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
	./$(BUILD_DIR)/test_config
	./$(BUILD_DIR)/test_json
	./$(BUILD_DIR)/test_repl
	./$(BUILD_DIR)/test_command
	./$(BUILD_DIR)/test_annotation
	./$(BUILD_DIR)/test_exec
	sh test/test_comment_ratio.sh

check: all test
	@echo "==> clang-tidy"
	@$(CLANG_TIDY) src/*.cpp -- -std=c++17 -I include/ 2>&1 | grep "warning:" | grep -v "linenoise" && exit 1 || true
	@echo "==> pmccabe (complexity <= 10)"
	@pmccabe src/*.cpp | awk '$$1 > 10 {print; found=1} END {if (found) exit 1}'
	@echo "==> cppcheck"
	cppcheck --enable=all --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=unmatchedSuppression --suppress=normalCheckLevelMaxBranches --suppress=checkersReport --error-exitcode=1 -I include/ src/
	@echo "==> doxygen lint"
	@doxygen Doxyfile 2>&1 | grep "warning:" | grep -v "No output formats" && exit 1 || true
	@echo "==> index freshness"
	@sh scripts/build-index.sh > /dev/null && git diff --quiet INDEX.md || { echo "FAIL: INDEX.md is outdated. Run 'make index'"; exit 1; }
	@echo "==> coverage (>= 80%)"
	@sh test/test_coverage.sh
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
	cp hooks/pre-commit .git/hooks/pre-commit
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
	@sh test/test_comment_ratio.sh

help:
	@echo "Usage:"
	@echo "  make           build the project"
	@echo "  make run       build and run"
	@echo "  make quick     incremental build + tests (fast)"
	@echo "  make test      full build + tests"
	@echo "  make check     run all quality checks"
	@echo "  make install   install git hooks"
	@echo "  make clean     remove build artifacts"
