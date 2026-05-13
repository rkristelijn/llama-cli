#!/usr/bin/env bash
#
# lint-scripts.sh — Validate shell script conventions.
#
# Checks: shebang, safety flags, kebab-case, header comment, subdirectory.
#
# Usage:
#   bash scripts/check/lint-scripts.sh
#
# @see docs/tools/shell-scripts.md
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Source cpm ui.sh if available, otherwise define minimal fallback
if [[ -f "${CPM_UI:-lib/cpm/shell/ui.sh}" ]]; then
  source "${CPM_UI:-lib/cpm/shell/ui.sh}"
else
  print_step() { echo "  $2 $3${4:+ $4}"; }
  print_header() { echo "==> $1"; }
  print_error() { echo "  ERROR: $1"; }
  print_warning() { echo "  WARNING: $1"; }
fi
failures=0

fail() {
  print_error "$1"
  ((failures++)) || true
}

main() {
  print_header "Checking script conventions..."

  while IFS= read -r script; do
    local name dir line1
    name="$(basename "${script}")"
    dir="$(dirname "${script}")"

    # Must be in a subdirectory of scripts/
    if [[ "${dir}" == "scripts" ]]; then
      fail "${script}: must be in a subdirectory (scripts/check/, scripts/dev/, etc.)"
    fi

    # Kebab-case filename (allow dots for extensions)
    if [[ "${name}" =~ [A-Z_] ]]; then
      fail "${script}: filename must be kebab-case (got: ${name})"
    fi

    # Shebang
    line1="$(head -1 "${script}")"
    if [[ "${line1}" != "#!/usr/bin/env bash" ]]; then
      fail "${script}: shebang must be #!/usr/bin/env bash"
    fi

    # Safety flags
    grep -q 'set -o errexit' "${script}" || fail "${script}: missing set -o errexit"
    grep -q 'set -o nounset' "${script}" || fail "${script}: missing set -o nounset"
    grep -q 'set -o pipefail' "${script}" || fail "${script}: missing set -o pipefail"

    # Syntax check (fastest possible shift-left)
    if ! bash -n "${script}" 2>/dev/null; then
      fail "${script}: syntax error (bash -n)"
    fi

    # Header comment in first 5 lines (excluding shebang)
    if ! head -5 "${script}" | tail -4 | grep -q '^#'; then
      fail "${script}: missing header comment in first 5 lines"
    fi

    # Max 3 exit points per script (1 usage + 1 tool-check + 1 error is normal)
    # Scripts can opt out with: # lint-exempt: max-exits
    if ! grep -q 'lint-exempt: max-exits' "${script}"; then
      local exit_count
      exit_count=$(grep -c 'exit [0-9]' "${script}" 2>/dev/null) || exit_count=0
      if [[ $exit_count -gt 3 ]]; then
        echo "  WARN: ${script}: ${exit_count} exit points (prefer max 3 — use a die() function)"
      fi
    fi

  done < <(find scripts -name '*.sh' -type f | sort)

  # ShellCheck static analysis (shift-left: catch issues before CI)
  if command -v shellcheck >/dev/null 2>&1; then
    echo ""
    echo "  --- shellcheck ---"
    local sc_fails=0
    # Smart mode: only check scripts changed vs main (fast for pre-push)
    local sc_files
    sc_files=$(git diff --name-only main...HEAD -- 'scripts/*.sh' 'scripts/**/*.sh' 2>/dev/null || true)
    if [[ -z "$sc_files" ]]; then
      echo "  ✓ shellcheck (no changed scripts vs main)"
    else
      while IFS= read -r script; do
        [[ -z "$script" || ! -f "$script" ]] && continue
        if ! shellcheck -S error "$script" >/dev/null 2>&1; then
          local issues
          issues=$(shellcheck -S error -f gcc "$script" 2>/dev/null | head -3)
          echo "  WARN: $script"
          echo "$issues" | sed 's/^/    /'
          sc_fails=$((sc_fails + 1))
        fi
      done <<<"$sc_files"
      if [[ $sc_fails -eq 0 ]]; then
        echo "  ✓ all scripts pass shellcheck"
      else
        echo "  $sc_fails script(s) have shellcheck warnings"
      fi
    fi
  fi

  # Extra lint rules (catches what ShellCheck misses, aligned with SonarCloud)
  echo ""
  echo "  --- extra rules ---"
  local extra_warns=0
  while IFS= read -r script; do
    [[ -z "$script" || ! -f "$script" ]] && continue
    # Bare return without exit code (SonarCloud: add explicit return statement)
    local bare_returns
    bare_returns=$(grep -c '^\s*return$' "$script" 2>/dev/null) || bare_returns=0
    if [[ $bare_returns -gt 0 ]]; then
      echo "  WARN: $script: $bare_returns bare return(s) — use 'return 0' or 'return 1'"
      extra_warns=$((extra_warns + 1))
    fi
  done <<<"$sc_files"
  if [[ $extra_warns -eq 0 ]]; then
    echo "  ✓ no extra lint issues"
  else
    echo "  $extra_warns script(s) have extra lint warnings"
  fi

  # Check that all Makefile script targets produce a log file
  echo ""
  echo "  --- log coverage ---"
  local missing_log=0
  while IFS= read -r target; do
    # Extract the full recipe from the target
    local recipe
    recipe=$(awk "/^${target}:/{found=1; next} found && /^\t/{print; next} found{exit}" Makefile)
    # Check if recipe runs a script but has no log output
    if echo "$recipe" | grep -q "bash scripts/" && ! echo "$recipe" | grep -q "tee .tmp/\|log_footer"; then
      echo "  [warn] target '${target}' runs a script but has no log output"
      missing_log=$((missing_log + 1))
    fi
  done < <(awk -F: '/^[a-z].*:.*##/{print $1}' Makefile | grep -v '^#')
  if [[ $missing_log -eq 0 ]]; then
    echo "  ✓ all script targets have log output"
  fi

  echo ""
  if ((failures > 0)); then
    echo "  ${failures} convention violation(s) found."
    return 1
  fi
  echo "  All scripts follow conventions."
}

main "$@"
