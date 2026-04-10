BUILD_DIR = build

.PHONY: all clean run test check install help

all:
	cmake -B $(BUILD_DIR) -S .
	cmake --build $(BUILD_DIR)

run: all
	./$(BUILD_DIR)/llama-cli

test: all
	cmake --build $(BUILD_DIR) --target test_config
	./$(BUILD_DIR)/test_config
	sh test/test_comment_ratio.sh

check: all
	@echo "==> cppcheck"
	cppcheck --enable=all --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=unmatchedSuppression --error-exitcode=1 -I include/ src/
	@echo "==> semgrep"
	PATH="$$HOME/.local/bin:$$PATH" semgrep scan --config auto --error
	@echo "==> gitleaks"
	gitleaks detect --source .
	@echo "All checks passed."

install:
	cp hooks/pre-commit .git/hooks/pre-commit
	chmod +x .git/hooks/pre-commit
	@echo "Git hooks installed."

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
