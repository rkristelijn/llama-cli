BUILD_DIR = build

.PHONY: all clean run test check install help

all:
	cmake -B $(BUILD_DIR) -S .
	cmake --build $(BUILD_DIR)

run: all
	./$(BUILD_DIR)/llama-cli

test: all
	cmake --build $(BUILD_DIR) --target test_config
	cmake --build $(BUILD_DIR) --target test_json
	cmake --build $(BUILD_DIR) --target test_repl
	./$(BUILD_DIR)/test_config
	./$(BUILD_DIR)/test_json
	./$(BUILD_DIR)/test_repl
	sh test/test_comment_ratio.sh

check: all test
	@echo "==> cppcheck"
	cppcheck --enable=all --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=unmatchedSuppression --suppress=normalCheckLevelMaxBranches --suppress=checkersReport --error-exitcode=1 -I include/ src/
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

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Usage:"
	@echo "  make           build the project"
	@echo "  make run       build and run"
	@echo "  make test      run tests"
	@echo "  make check     run all quality checks"
	@echo "  make install   install git hooks"
	@echo "  make clean     remove build artifacts"
