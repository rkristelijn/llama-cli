#!/usr/bin/env bash
#
# summarize-headers.sh — Generate one-line summaries for project files using Ollama.
#
# Only processes files that lack a good summary: missing @brief (C++/H),
# missing frontmatter summary (Markdown with frontmatter), or a fallback
# first-line shorter than 20 chars.
#
# Usage:
#   bash scripts/dev/summarize-headers.sh [--dry-run] [--model=MODEL]
#
# Environment:
#   OLLAMA_HOST  — Ollama host (default: localhost)
#   OLLAMA_PORT  — Ollama port (default: 11434)

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
MODEL="${MODEL:-llama3.2:3b}"
# OLLAMA_HOST may include port (e.g. localhost:11434)
_HOST="${OLLAMA_HOST:-localhost:11434}"
HOST="${_HOST%%:*}"
PORT="${OLLAMA_PORT:-${_HOST##*:}}"
PORT="${PORT:-11434}"
DRY_RUN=false

for arg in "$@"; do
  case "$arg" in
  --dry-run) DRY_RUN=true ;;
  --model=*) MODEL="${arg#--model=}" ;;
  esac
done

# --- Progress bar (from gitlab-port-scanner) ---

percentBar() {
  local prct totlen=$((8 * $2))
  local -a chars=('▏' '▎' '▍' '▌' '▋' '▊' '▉')
  printf -v prct %.2f "$1"
  prct=${prct/./}
  prct=$((10#$prct * totlen / 10000))
  local remainder=$((prct % 8))
  local lastchar=""
  [[ $remainder -gt 0 ]] && lastchar="${chars[$((remainder - 1))]}"
  local barstring blankstring
  printf -v barstring '%*s' $((prct / 8)) ''
  barstring="${barstring// /█}$lastchar"
  printf -v blankstring '%*s' $(((totlen - prct) / 8)) ''
  printf -v "$3" '%s%s' "$barstring" "$blankstring"
}

# --- Check if file needs a summary ---

needs_summary() {
  local file="$1"
  case "$file" in
  *.md)
    # No frontmatter at all? Can't inject — skip
    if ! head -1 "$file" | grep -q '^---$'; then return 1; fi
    # Has frontmatter but no summary field? Needs one.
    if ! sed -n '/^---$/,/^---$/p' "$file" 2>/dev/null | grep -q '^summary:'; then
      return 0
    fi
    # Has summary but too short (<200 chars)? Needs update.
    local existing
    existing=$(sed -n '/^---$/,/^---$/p' "$file" | grep '^summary:' | sed 's/^summary: *//')
    if [[ ${#existing} -lt 200 ]]; then return 0; fi
    return 1
    ;;
  *.cpp | *.h)
    # Has @brief in doxygen block?
    head -10 "$file" | grep -q '@brief' && return 1
    # Has a meaningful // comment on line 1? (≥ 20 chars)
    local first
    first=$(head -1 "$file" | sed 's|^// *||')
    [[ ${#first} -ge 20 ]] && return 1
    return 0
    ;;
  *.sh)
    # Line 3 should have a description (after shebang + blank)
    sed -n '3p' "$file" | grep -qE '^# .{15,}' && return 1
    return 0
    ;;
  esac
  return 1
}

# --- Check if file was modified (summary may be stale) ---

is_modified() {
  local file="$1"
  # Changed vs main or vs last commit
  git diff --name-only main...HEAD -- "$file" 2>/dev/null | grep -q . && return 0
  git diff --name-only HEAD -- "$file" 2>/dev/null | grep -q . && return 0
  return 1
}

# --- Ask Ollama for a one-line summary ---

ask_ollama() {
  local content="$1"
  local existing_summary="${2:-}"
  local prompt

  if [[ -n "$existing_summary" ]]; then
    # File has been modified — ask LLM to verify or update
    prompt="A file has this summary:\n\"${existing_summary}\"\n\nHere is the current file content:\n${content}\n\nIf the summary still accurately describes the file, respond with exactly: up to date\nOtherwise, write a new summary in exactly one sentence, 200-300 characters. No quotes, no filename. Be specific and technical."
  else
    prompt="Summarize this file in exactly one sentence, 200-300 characters. No quotes, no filename. Be specific and technical about what it implements or documents:\n\n${content}"
  fi

  local response
  for _ in 1 2; do
    response=$(curl -s --max-time 45 "http://${HOST}:${PORT}/api/generate" \
      -d "$(jq -n --arg model "$MODEL" --arg prompt "$prompt" \
        '{model: $model, prompt: $prompt, stream: false}')" |
      jq -r '.response // empty' 2>/dev/null)
    [[ -n "$response" ]] && break
    sleep 2
  done

  # Clean: single line, trim whitespace
  response=$(echo "$response" | tr '\n' ' ' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')

  # If LLM says it's up to date, signal no change needed
  if echo "$response" | grep -qi "up.to.date\|remains accurate\|still accurate\|summary is correct\|accurately describes"; then
    echo "UP_TO_DATE"
    return 0
  fi

  # Reject responses that are meta-commentary rather than a summary
  if echo "$response" | grep -qi "^Here is\|^The summary\|^This is a\|^The current\|^A rewritten"; then
    echo ""
    return 0
  fi

  # Cap at 300 chars
  echo "$response" | cut -c1-300
}

# --- Portable sed -i (BSD vs GNU) ---

sed_inplace() {
  if sed --version >/dev/null 2>&1; then
    sed -i "$@"
  else
    sed -i '' "$@"
  fi
}

# --- Inject summary into file header ---

inject_summary() {
  local file="$1" summary="$2"
  [[ "$DRY_RUN" == "true" ]] && return

  case "$file" in
  *.md)
    if head -1 "$file" | grep -q '^---$'; then
      if sed -n '/^---$/,/^---$/p' "$file" | grep -q '^summary:'; then
        # Replace existing summary
        sed_inplace "s|^summary: .*|summary: ${summary}|" "$file"
      else
        # Add summary: into existing frontmatter
        sed_inplace "/^---$/,/^---$/{
          /^---$/{
            n
            /^summary:/!i\\
summary: ${summary}
          }
        }" "$file"
      fi
    fi
    ;;
  *.cpp | *.h)
    if head -1 "$file" | grep -q '^/\*\*'; then
      if head -10 "$file" | grep -q '@brief'; then
        sed_inplace "s|@brief .*|@brief ${summary}|" "$file"
      else
        sed_inplace "2a\\
 * @brief ${summary}" "$file"
      fi
    else
      # No doxygen block — add a comment at line 1 for .h files
      if [[ "$file" == *.h ]]; then
        local existing
        existing=$(head -1 "$file")
        if [[ "$existing" == "//"* ]]; then
          # Replace first comment line
          sed_inplace "1s|// .*|// ${summary}|" "$file"
        fi
      fi
    fi
    ;;
  esac
}

# --- Main ---

main() {
  echo "==> Summarizing file headers with ${MODEL}..."

  # Verify Ollama
  if ! curl -s --max-time 5 "http://${HOST}:${PORT}/api/tags" >/dev/null 2>&1; then
    echo "Error: cannot reach Ollama at ${HOST}:${PORT}"
    exit 1
  fi

  # Phase 1: collect files that need summaries or have been modified
  local candidates=()
  local modified=()
  while IFS= read -r -d '' f; do
    if needs_summary "$f"; then
      candidates+=("$f")
    elif is_modified "$f"; then
      # File has a summary but content changed — verify it's still accurate
      case "$f" in
      *.md)
        if head -1 "$f" | grep -q '^---$'; then
          modified+=("$f")
        fi
        ;;
      esac
    fi
  done < <(find docs src scripts -type f \
    \( -name "*.md" -o -name "*.cpp" -o -name "*.h" -o -name "*.sh" \) \
    -print0 | sort -z)

  # Merge: candidates first, then modified files for verification
  local all_files=("${candidates[@]}" "${modified[@]}")
  local total=${#all_files[@]}
  if [[ $total -eq 0 ]]; then
    echo "  All files already have up-to-date summaries."
    return 0
  fi

  echo "  ${#candidates[@]} need summaries, ${#modified[@]} modified (verify)"
  echo "  dry-run: ${DRY_RUN}"
  echo ""

  # Phase 2: summarize
  local done_count=0 updated=0 skipped=0 failed=0
  local start_time elapsed avg_per_file eta_secs
  start_time=$(date +%s)
  for file in "${all_files[@]}"; do
    done_count=$((done_count + 1))
    local pct=$((done_count * 100 / total))
    local bar

    # Calculate ETA
    elapsed=$(($(date +%s) - start_time))
    if [[ $done_count -gt 1 && $elapsed -gt 0 ]]; then
      avg_per_file=$((elapsed / (done_count - 1)))
      eta_secs=$((avg_per_file * (total - done_count)))
      local eta_min=$((eta_secs / 60))
      local eta_sec=$((eta_secs % 60))
      printf -v eta_str "%dm%02ds" "$eta_min" "$eta_sec"
    else
      eta_str="--:--"
    fi

    percentBar "$pct" 30 bar
    printf "\r  [%s] %3d%% (%d/%d) ETA %s %-40s" "$bar" "$pct" "$done_count" "$total" "$eta_str" "$(basename "$file")"

    local content existing_summary=""
    content=$(head -100 "$file")

    # Get existing summary if this is a modified file
    case "$file" in
    *.md)
      existing_summary=$(sed -n '/^---$/,/^---$/p' "$file" | grep '^summary:' | sed 's/^summary: *//')
      ;;
    esac

    local summary
    summary=$(ask_ollama "$content" "$existing_summary")

    if [[ -z "$summary" ]]; then
      failed=$((failed + 1))
      printf " [FAIL: empty response]\n"
      continue
    fi

    if [[ "$summary" == "UP_TO_DATE" ]]; then
      skipped=$((skipped + 1))
      continue
    fi

    if [[ "$DRY_RUN" == "true" ]]; then
      printf "\n    → %s\n" "$summary"
    else
      inject_summary "$file" "$summary"
    fi
    updated=$((updated + 1))
  done

  printf "\r  [████████████████████████████████████████] 100%%                                    \n"
  echo ""
  elapsed=$(($(date +%s) - start_time))
  local total_min=$((elapsed / 60)) total_sec=$((elapsed % 60))
  echo "  updated: ${updated}  skipped: ${skipped}  failed: ${failed}  total: ${total}  (${total_min}m${total_sec}s)"

  if [[ "$DRY_RUN" == "false" && "$updated" -gt 0 ]]; then
    echo "  Run 'make index' to regenerate INDEX.md."
  fi
}

main "$@"
