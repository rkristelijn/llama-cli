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
  prct=${prct/.}
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
      # Has frontmatter summary? Good enough.
      sed -n '/^---$/,/^---$/p' "$file" 2>/dev/null | grep -q '^summary:' && return 1
      # No frontmatter? First heading or line ≥ 20 chars is fine (can't inject without frontmatter)
      if ! head -1 "$file" | grep -q '^---$'; then return 1; fi
      # Has frontmatter but no summary — check if first non-frontmatter line is useful
      local first
      first=$(awk '/^---$/{n++; next} n>=2{print; exit}' "$file" | sed 's/^[# ]*//' | cut -c1-80)
      [[ ${#first} -ge 20 ]] && return 1
      return 0
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

# --- Ask Ollama for a one-line summary ---

ask_ollama() {
  local content="$1"
  local prompt
  prompt="Summarize this file in exactly one line, max 100 characters. No quotes, no filename, just a plain description:\n\n${content}"

  local response
  response=$(curl -s --max-time 30 "http://${HOST}:${PORT}/api/generate" \
    -d "$(jq -n --arg model "$MODEL" --arg prompt "$prompt" \
      '{model: $model, prompt: $prompt, stream: false}')" \
    | jq -r '.response // empty' 2>/dev/null)

  # Clean: single line, trim whitespace, cap at 120 chars
  echo "$response" | tr '\n' ' ' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' | cut -c1-120
}

# --- Inject summary into file header ---

inject_summary() {
  local file="$1" summary="$2"
  [[ "$DRY_RUN" == "true" ]] && return

  case "$file" in
    *.md)
      if head -1 "$file" | grep -q '^---$'; then
        # Add summary: into existing frontmatter
        sed -i '' "/^---$/,/^---$/{
          /^---$/{
            n
            /^summary:/!i\\
summary: ${summary}
          }
        }" "$file"
      fi
      ;;
    *.cpp | *.h)
      if head -1 "$file" | grep -q '^/\*\*'; then
        if head -10 "$file" | grep -q '@brief'; then
          sed -i '' "s|@brief .*|@brief ${summary}|" "$file"
        else
          sed -i '' "2a\\
 * @brief ${summary}" "$file"
        fi
      else
        # No doxygen block — add a comment at line 1 for .h files
        if [[ "$file" == *.h ]]; then
          local existing
          existing=$(head -1 "$file")
          if [[ "$existing" == "//"* ]]; then
            # Replace first comment line
            sed -i '' "1s|// .*|// ${summary}|" "$file"
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
  if ! curl -s --max-time 5 "http://${HOST}:${PORT}/api/tags" > /dev/null 2>&1; then
    echo "Error: cannot reach Ollama at ${HOST}:${PORT}"
    exit 1
  fi

  # Phase 1: collect files that need summaries
  local candidates=()
  while IFS= read -r -d '' f; do
    if needs_summary "$f"; then
      candidates+=("$f")
    fi
  done < <(find docs src scripts -type f \
    \( -name "*.md" -o -name "*.cpp" -o -name "*.h" -o -name "*.sh" \) \
    -print0 | sort -z)

  local total=${#candidates[@]}
  if [[ $total -eq 0 ]]; then
    echo "  All files already have summaries."
    return
  fi

  echo "  ${total} files need summaries (dry-run: ${DRY_RUN})"
  echo ""

  # Phase 2: summarize
  local done_count=0 updated=0 failed=0
  for file in "${candidates[@]}"; do
    done_count=$((done_count + 1))
    local pct=$((done_count * 100 / total))
    local bar
    percentBar "$pct" 30 bar
    printf "\r  [%s] %3d%% (%d/%d) %-50s" "$bar" "$pct" "$done_count" "$total" "$(basename "$file")"

    local content
    content=$(head -100 "$file")
    local summary
    summary=$(ask_ollama "$content")

    if [[ -z "$summary" ]]; then
      failed=$((failed + 1))
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
  echo "  updated: ${updated}  failed: ${failed}  total: ${total}"

  if [[ "$DRY_RUN" == "false" && "$updated" -gt 0 ]]; then
    echo "  Run 'make index' to regenerate INDEX.md."
  fi
}

main "$@"
