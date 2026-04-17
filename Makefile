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

.PHONY: all build clean run start log test test-unit test-e2e e2e check full-check check-ai format format-check install hooks help quick index comment-ratio pipeline-status pr-status pr pr-feedback download-issues check-deps live tidy lint complexity docs sast sast-secret sast-security coverage coverage-folder todo

check-deps:
	@command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake not found. Run 'make setup' first."; exit 1; }

all: check-deps
	@cmake -B $(BUILD_DIR) -S . > /dev/null
	@cmake --build $(BUILD_DIR) --target llama-cli > /dev/null
	@cp $(BUILD_DIR)/llama-cli .

build: all

run: all
	./$(BUILD_DIR)/llama-cli $(ARGS)

start: all
	./$(BUILD_DIR)/llama-cli $(ARGS)

log:
	@sh scripts/log-viewer.sh $(ARGS)

test-unit: all
	@echo "==> make test-unit"
	@cmake --build $(BUILD_DIR) --target test_config > /dev/null
	@cmake --build $(BUILD_DIR) --target test_json > /dev/null
	@cmake --build $(BUILD_DIR) --target test_repl > /dev/null
	@cmake --build $(BUILD_DIR) --target test_command > /dev/null
	@cmake --build $(BUILD_DIR) --target test_annotation > /dev/null
	@cmake --build $(BUILD_DIR) --target test_exec > /dev/null
	@./$(BUILD_DIR)/test_config --quiet
	@./$(BUILD_DIR)/test_json --quiet
	@./$(BUILD_DIR)/test_repl --quiet
	@./$(BUILD_DIR)/test_command --quiet
	@./$(BUILD_DIR)/test_annotation --quiet
	@./$(BUILD_DIR)/test_exec --quiet
	@echo "  [done] test-unit"

test: test-unit

e2e: test-e2e

# Live integration test with real LLM (requires running Ollama)
live: all
	@bash e2e/test_live.sh $(BUILD_DIR)/llama-cli

test-e2e: build
	@echo "==> make e2e"
	@for t in e2e/*.sh; do \
		case "$$t" in *test_live*|*helpers*) continue;; esac; \
		bash "$$t" $(BUILD_DIR)/llama-cli > /dev/null || { echo "FAIL: $$t"; exit 1; }; \
	done
	@echo "  [done] e2e"

tidy: all
	@if [ "$(SMART)" = "1" ]; then \
		echo "==> make tidy (smart incremental mode)"; \
		branch=$$(git rev-parse --abbrev-ref HEAD); \
		diff_base=$$( [ "$$branch" = "main" ] && echo "HEAD^" || echo "origin/main" ); \
		files=$$(git diff --name-only $$diff_base | grep "\.cpp$$" | grep "^src/" || true); \
		if [ -z "$$files" ]; then \
			echo "  [skip] no changed files vs $$diff_base"; \
		else \
			for dir in src $$(find src -maxdepth 1 -mindepth 1 -type d); do \
				dir_files=$$(echo "$$files" | grep "^$$dir/[^/]*\.cpp$$" || true); \
				if [ -n "$$dir_files" ]; then \
					echo "  [checking] $$dir/ ($$(echo $$dir_files | wc -w) files)"; \
					$(CLANG_TIDY) --config-file=.config/.clang-tidy $$dir_files -- -std=c++17 -I src/ 2>&1 | grep "warning:" | grep -v "linenoise\|SCENARIO\|cognitive complexity\|identifier-naming\|logging/logger.*function-size" && exit 1 || true; \
				fi; \
			done; \
		fi; \
	else \
		echo "==> make tidy (full mode)"; \
		find src -name '*.cpp' -print0 | xargs -0 $(CLANG_TIDY) --config-file=.config/.clang-tidy -- -std=c++17 -I src/ 2>&1 | grep "warning:" | grep -v "linenoise\|SCENARIO\|cognitive complexity\|identifier-naming\|logging/logger.*function-size" && exit 1 || true; \
	fi
	@echo "  [done] tidy"

lint: all
	@echo "==> make lint (running cppcheck...)"
	@cppcheck --enable=all --suppress=missingIncludeSystem --suppress=missingInclude --suppress=unusedFunction --suppress=unmatchedSuppression --suppress=useStlAlgorithm --suppress=knownConditionTrueFalse:*_it.cpp --suppress=knownConditionTrueFalse:*_test.cpp --error-exitcode=1 -I src/ src/ 2>&1 | grep -v "Checking\|files checked" || true
	@echo "  [done] lint"

complexity: all
	@echo "==> make complexity (running pmccabe...)"
	@find src -name '*.cpp' | xargs pmccabe 2>/dev/null | while read line; do \
		file=$$(echo $$line | awk '{print $$6}' | cut -d'(' -f1); \
		lno=$$(echo $$line | awk '{print $$6}' | cut -d'(' -f2 | cut -d')' -f1); \
		if ! head -n $$lno $$file | tail -n 6 | grep -q "pmccabe:skip-complexity\|clang-tidy:skip-complexity"; then \
			echo $$line | awk '$$1 > 10 {print; exit 1}'; \
		fi; \
	done || exit 1
	@echo "  [done] complexity"

sast-security:
	@echo "==> make sast-security (semgrep...)"
	@if command -v semgrep >/dev/null; then \
		semgrep scan --config auto --error --quiet 2>&1 | grep -v "┌────\|Semgrep CLI\|└─────────────" || true; \
	fi
	@echo "  [done] sast-security"

sast-secret:
	@echo "==> make sast-secret (gitleaks...)"
	@if command -v gitleaks >/dev/null; then \
		gitleaks detect --source . --log-level error --no-banner; \
	fi
	@echo "  [done] sast-secret"

sast: sast-security sast-secret

docs:
	@echo "==> make docs (generating doxygen...)"
	@doxygen .config/Doxyfile 2>&1 | grep "warning:" | grep -v "No output formats\|Unsupported xml\|falsely parses" && exit 1 || true
	@echo "  [done] docs"

check: format-check tidy complexity lint docs index
	@$(MAKE) -s coverage-folder
	@$(MAKE) -s sast
	@echo "==> make comment-ratio"
	@bash scripts/test_comment_ratio.sh | grep "PASS" || bash scripts/test_comment_ratio.sh
	@echo "==> make todo"
	@$(MAKE) -s todo | grep -v "==> make todo"
	@echo ""
	@echo "All checks passed."

full-check:
	@$(MAKE) FULL=1 check

check-ai:
	@$(MAKE) -s check 2>&1 | grep -E "^\s*([0-9]+|src/|==>|FAIL|All checks passed|knownCondition|always false|too many|warning:|error:)" | grep -v "^$$"

setup:
	sh scripts/setup.sh

install: all
	@cp $(BUILD_DIR)/llama-cli /usr/local/bin/llama-cli
	@echo "Installed to /usr/local/bin/llama-cli"

hooks:
	@cp .config/pre-commit .git/hooks/pre-commit
	@chmod +x .git/hooks/pre-commit
	@echo "Git hooks installed."

todo:
	@# Shows TODO items from all markdown files and technical debt in code.
	@# Bolds parent tasks that have indented sub-tasks.
	@echo "==> Markdown TODOs"
	@find . -name "*.md" -not -path "./build/*" -not -path "./.git/*" -exec awk ' \
		FNR == 1 { \
			if (prev_line != "") printf "%s:%d:%s\n", prev_file, prev_lnum, prev_line; \
			prev_line = ""; \
		} \
		/- \[ \]/ { \
			match($$0, /[^ ]/); \
			curr_indent = RSTART; \
			if (prev_line != "") { \
				if (curr_indent > prev_indent) { \
					printf "%s:%d:\033[1m%s\033[0m\n", FILENAME, prev_lnum, prev_line; \
				} else { \
					printf "%s:%d:%s\n", FILENAME, prev_lnum, prev_line; \
				} \
			} \
			prev_line = $$0; \
			prev_indent = curr_indent; \
			prev_lnum = FNR; \
			prev_file = FILENAME; \
		} \
		END { \
			if (prev_line != "") printf "%s:%d:%s\n", FILENAME, prev_lnum, prev_line; \
		}' {} +
	@echo ""
	@echo "==> Code TODOs"
	@grep -rn "TODO\|FIXME\|HACK\|XXX" src/ include/ --include="*.cpp" --include="*.h" 2>/dev/null || true

index:
	@echo "==> make index"
	@sh scripts/build-index.sh

format:
	@echo "==> make format"
	@find src -name '*.cpp' -o -name '*.h' | xargs clang-format -i --style=file:.config/.clang-format

format-check:
	@echo "==> make format-check"
	@find src -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run -Werror --style=file:.config/.clang-format
	@echo "format-check: OK"

comment-ratio:
	@cloc src/ --not-match-f='(_test|_it)\.cpp$$' --by-file --csv --quiet \
	  | grep -v "^language\|^SUM\|^http" \
	  | awk -F',' 'NF==5 && $$5>0 {ratio=int($$4/($$4+$$5)*100); printf "%d%%\t%s\n", ratio, $$2}' \
	  | sort -n

coverage:
	@echo "==> make coverage (configuring with --coverage...)"
	@cmake -B $(BUILD_DIR) -S . -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage" > /dev/null
	@cmake --build $(BUILD_DIR) > /dev/null
	@echo "==> make coverage (running tests...)"
	@./$(BUILD_DIR)/test_config --quiet
	@./$(BUILD_DIR)/test_json --quiet
	@./$(BUILD_DIR)/test_repl --quiet
	@./$(BUILD_DIR)/test_command --quiet
	@./$(BUILD_DIR)/test_annotation --quiet
	@./$(BUILD_DIR)/test_exec --quiet
	@echo "  [done] coverage"

coverage-folder: coverage
	@echo "==> make coverage-folder"
	@for dir in src $$(find src -maxdepth 1 -mindepth 1 -type d); do \
		files=$$(ls $(BUILD_DIR)/CMakeFiles/*.dir/$$dir/*.cpp.o 2>/dev/null); \
		if [ -n "$$files" ]; then \
			printf "  %-20s " "$$dir/"; \
			gcov -n $$files 2>/dev/null \
			| sed 's/,/./g' \
			| awk '/Lines executed/ { \
				split($$2, a, ":"); split(a[2], b, "%"); \
				exec_lines += b[1] * $$4 / 100; \
				total_lines += $$4; \
			} END { \
				if (total_lines > 0) printf "%.2f%%\n", (exec_lines / total_lines) * 100; \
				else print "N/A"; \
			}' || echo "N/A"; \
		fi; \
	done
	@echo "  [done] coverage-folder"

clean:
	rm -rf $(BUILD_DIR) llama-cli

quick: all
	@./$(BUILD_DIR)/test_config --quiet
	@./$(BUILD_DIR)/test_json --quiet
	@./$(BUILD_DIR)/test_repl --quiet
	@./$(BUILD_DIR)/test_command --quiet
	@./$(BUILD_DIR)/test_annotation --quiet
	@./$(BUILD_DIR)/test_exec --quiet
	@bash scripts/test_comment_ratio.sh | grep "PASS" || bash scripts/test_comment_ratio.sh

prepush:
	@changed=$$(git diff --name-only origin/main...HEAD); \
	if echo "$$changed" | grep -qE '\.(cpp|h)$$'; then \
		echo "==> make check (code changed)"; \
		$(MAKE) -s check; \
	else \
		echo "==> make index (docs only)"; \
		$(MAKE) -s index; \
		git diff --quiet INDEX.md || { echo "FAIL: INDEX.md outdated"; exit 1; }; \
		echo "All checks passed."; \
	fi

help:
	@echo "Usage:"
	@echo "  make                build the project"
	@echo "  make run            build and run (alias: make start)"
	@echo "  make quick          incremental build + tests (fast)"
	@echo "  make test           unit tests"
	@echo "  make e2e            end-to-end tests"
	@echo "  make check          run smart quality checks (default)"
	@echo "  make full-check     run exhaustive quality checks (used on main)"
	@echo "  make sast           run all static analysis (security & secrets)"
	@echo "  make sast-security  run security analysis (semgrep)"
	@echo "  make sast-secret    run secret scanning (gitleaks)"
	@echo "  make tidy           run incremental clang-tidy"
	@echo "  make coverage       generate coverage report"
	@echo "  make coverage-folder show coverage summary per folder"
	@echo "  make clean          remove build artifacts"
	@echo "  make todo           show TODO items from docs and code"
