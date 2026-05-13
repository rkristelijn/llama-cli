#!/usr/bin/env bash
# check-slop.sh — Detect common AI-generated code slop patterns.
# Scans git diff against main for signs of low-quality AI output.

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
print_header "checking for AI slop patterns..."
set +e

DIFF=$(git diff main...HEAD -- 'src/*.cpp' 'src/*.h' 'src/**/*.cpp' 'src/**/*.h' 2>/dev/null || true)
if [[ -z "$DIFF" ]]; then
  print_step "" "$(basename "$0" .sh)" skip "no C++ changes vs main"
  exit 0
fi

WARNINGS=0

# 1. Obvious comments that restate the code
count=$(echo "$DIFF" | grep -c '^\+.*// \(set\|get\|return\|increment\|decrement\|initialize\|assign\) ' || true)
if [[ $count -gt 5 ]]; then
  echo "  ⚠ Redundant comments: $count lines restate what the code does"
  echo "    → Comments should explain WHY, not WHAT"
  ((WARNINGS++)) || true || true
fi

# 2. Empty catch blocks
count=$(echo "$DIFF" | grep -c '^\+.*catch.*{[[:space:]]*}' || true)
if [[ $count -gt 0 ]]; then
  echo "  ⚠ Empty catch blocks: $count (errors silently swallowed)"
  echo "    → Log or handle the error, don't ignore it"
  ((WARNINGS++)) || true
fi

# 3. Defensive nullptr checks with "should never" comments
count=$(echo "$DIFF" | grep -c '^\+.*if.*!=.*nullptr.*//.*should never' || true)
if [[ $count -gt 0 ]]; then
  echo "  ⚠ Paranoid nullptr checks: $count with 'should never happen' comments"
  echo "    → If it should never happen, use an assert instead"
  ((WARNINGS++)) || true
fi

# 4. Too many TODO/FIXME in new code
count=$(echo "$DIFF" | grep -c '^\+.*// \(TODO\|FIXME\|HACK\|XXX\):' || true)
if [[ $count -gt 3 ]]; then
  echo "  ⚠ Unresolved markers: $count TODO/FIXME/HACK in new code"
  echo "    → Resolve before committing, or move to TODO.md"
  ((WARNINGS++)) || true
fi

# 5. Excessive blank lines
count=$(echo "$DIFF" | grep -c '^\+$' || true)
total_added=$(echo "$DIFF" | grep -c '^\+' || true)
if [[ $total_added -gt 0 ]]; then
  blank_ratio=$((count * 100 / total_added))
  if [[ $blank_ratio -gt 15 ]]; then
    echo "  ⚠ Blank line bloat: ${blank_ratio}% of additions are empty lines (max 15%)"
    echo "    → Remove unnecessary spacing"
    ((WARNINGS++)) || true
  fi
fi

# 6. Repeated boilerplate (same line 4+ times)
dupes=$(echo "$DIFF" | grep '^\+' | sed 's/^\+//' | grep -v '^$' | grep -v '^[[:space:]]*[{}]$' | sort | uniq -c | sort -rn | awk '$1 >= 4 && length($0) > 20 {print}' | head -3)
if [[ -n "$dupes" ]]; then
  echo "  ⚠ Repeated boilerplate (same line 4+ times):"
  echo "$dupes" | while read -r line; do
    echo "    $line"
  done
  echo "    → Extract to a function or loop"
  ((WARNINGS++)) || true
fi

# 7. Trivial docstrings on short functions (/// comment above a 1-3 line function)
count=$(echo "$DIFF" | grep -c '^\+.*/// \(Get\|Set\|Return\|Check\|Create\|Initialize\) ' || true)
if [[ $count -gt 8 ]]; then
  echo "  ⚠ Trivial docstrings: $count 'Get/Set/Return/Check' style comments"
  echo "    → Only document non-obvious behavior, not the function name"
  ((WARNINGS++)) || true
fi

# 8. Over-commenting: more comment lines than code lines in additions
comment_lines=$(echo "$DIFF" | grep -c '^\+[[:space:]]*//' || true)
code_lines=$(echo "$DIFF" | grep -c '^\+' | head -1 || true)
if [[ $code_lines -gt 50 ]]; then
  comment_pct=$((comment_lines * 100 / code_lines))
  if [[ $comment_pct -gt 40 ]]; then
    echo "  ⚠ Over-commenting: ${comment_pct}% of new lines are comments (max 40%)"
    echo "    → Comments should be sparse and meaningful, not a running narration"
    ((WARNINGS++)) || true
  fi
fi

# 9. Single-implementation abstractions (abstract class with only 1 subclass in diff)
abstract_count=$(echo "$DIFF" | grep -c '^\+.*virtual.*= 0' || true)
impl_count=$(echo "$DIFF" | grep -c '^\+.*override' || true)
if [[ $abstract_count -gt 3 && $impl_count -le $abstract_count ]]; then
  echo "  ⚠ Abstraction inflation: $abstract_count virtual methods but only $impl_count overrides"
  echo "    → Don't create interfaces until you have 2+ implementations (YAGNI)"
  ((WARNINGS++)) || true
fi

# 10. Cargo cult: unnecessary std::move on trivial types or string literals
count=$(echo "$DIFF" | grep -c '^\+.*std::move("' || true)
if [[ $count -gt 0 ]]; then
  echo "  ⚠ Cargo cult: $count std::move on string literals (does nothing)"
  echo "    → std::move on literals/temporaries is pointless"
  ((WARNINGS++)) || true
fi

# 11. Semantic duplication: same function body appearing in multiple new functions
func_bodies=$(echo "$DIFF" | grep '^\+' | grep -E '^\+\s+(return|if \(|for \(|while \()' | sort | uniq -d | wc -l)
if [[ $func_bodies -gt 10 ]]; then
  echo "  ⚠ Semantic duplication: $func_bodies repeated logic patterns"
  echo "    → Extract shared logic into a helper function"
  ((WARNINGS++)) || true
fi

# 12. AI tell-words in comments (expanded from antislop-sampler research)
count=$(echo "$DIFF" | grep -ci '^\+.*//.*\(delve\|comprehensive\|leverage\|utilize\|facilitate\|streamline\|robust\|seamless\|holistic\|synergy\|paradigm\|ecosystem\|orchestrat\|intricacies\|nuanced\|pivotal\|foster\)' || true)
if [[ $count -gt 3 ]]; then
  echo "  ⚠ AI vocabulary: $count comments use AI-typical words (delve/leverage/seamless/orchestrate...)"
  echo "    → Use plain language: 'use' not 'utilize', 'strong' not 'robust'"
  ((WARNINGS++)) || true
fi

# 13. Pass-through wrappers (function body is just a call to another function)
# Source: slop-scan's top rule — AI generates wrappers that add no value
passthrough=$(echo "$DIFF" | grep -c '^\+.*return [a-zA-Z_]*(' || true)
if [[ $passthrough -gt 15 ]]; then
  echo "  ⚠ Possible pass-through wrappers: $passthrough return-only lines"
  echo "    → Functions that just forward to another add indirection without value"
  ((WARNINGS++)) || true
fi

# 14. Catch-log-rethrow (catch that only logs and rethrows — adds nothing)
# Source: slop-guard, slop-scan error-swallowing rules
catch_log=$(echo "$DIFF" | grep -c '^\+.*\(std::cerr\|LOG_\|log(\).*\(throw\|rethrow\)' || true)
if [[ $catch_log -gt 2 ]]; then
  echo "  ⚠ Catch-log-rethrow: $catch_log blocks that only log and rethrow"
  echo "    → Either handle the error or let it propagate — don't just log and rethrow"
  ((WARNINGS++)) || true
fi

# 15. AI class/function naming: excessive Helper/Manager/Utils/Wrapper/Normalize
# Source: r/dotnet community observation, slop-guard over-abstraction rules
ai_names=$(echo "$DIFF" | grep -ci '^\+.*\(class\|struct\|void\|auto\).*\(Helper\|Manager\|Utils\|Wrapper\|Normalize\|Processor\|Handler\)' || true)
if [[ $ai_names -gt 5 ]]; then
  echo "  ⚠ AI naming patterns: $ai_names uses of Helper/Manager/Utils/Wrapper/Normalize"
  echo "    → Name by what it does, not by generic role suffixes"
  ((WARNINGS++)) || true
fi

# 16. Debug output left behind (std::cout/cerr/printf in non-debug code)
# Source: remove-ai-slop, slop-guard
debug_out=$(echo "$DIFF" | grep -c '^\+.*\(std::cout\|std::cerr\|printf(\|fprintf(\)' || true)
if [[ $debug_out -gt 5 ]]; then
  echo "  ⚠ Debug output left behind: $debug_out print statements in new code"
  echo "    → Use the logging framework, not raw stdout/stderr"
  ((WARNINGS++)) || true
fi

# 17. Uniform line length (low variance = AI monotony signal)
# Source: Nirob et al. 2026 "Whitespaces Don't Lie", Artificial Ignorance article
if [[ $total_added -gt 50 ]]; then
  lengths=$(echo "$DIFF" | grep '^\+' | grep -v '^\+$' | awk '{print length}')
  if [[ -n "$lengths" ]]; then
    # Calculate standard deviation of line lengths
    stddev=$(echo "$lengths" | awk '{sum+=$1; sumsq+=$1*$1; n++} END {if(n>10) printf "%.0f", sqrt(sumsq/n - (sum/n)^2); else print 999}')
    if [[ $stddev -lt 8 && $stddev -ne 999 ]]; then
      echo "  ⚠ Uniform line length: stddev=$stddev (AI code tends toward monotonous structure)"
      echo "    → Human code naturally varies in line length; consider if this was bulk-generated"
      ((WARNINGS++)) || true
    fi
  fi
fi

# --- Documentation slop (markdown files) ---
MD_DIFF=$(git diff main...HEAD -- '*.md' 2>/dev/null || true)
if [[ -n "$MD_DIFF" ]]; then
  # 18. AI tell-words in docs (expanded word list from antislop-sampler research)
  md_tells=$(echo "$MD_DIFF" | grep -ci '^\+.*\(delve\|comprehensive\|leverage\|utilize\|facilitate\|streamline\|in today.s rapidly\|it.s important to note\|seamless\|holistic\|paradigm\|ecosystem\|pivotal\|foster\|intricacies\)' || true)
  if [[ $md_tells -gt 5 ]]; then
    echo "  ⚠ AI vocabulary in docs: $md_tells uses of AI-typical words"
    echo "    → Write like a human: direct, specific, no filler"
    ((WARNINGS++)) || true
  fi

  # 19. Filler paragraphs (expanded from community research)
  filler=$(echo "$MD_DIFF" | grep -ci '^\+.*\(in summary\|as mentioned\|it is worth noting\|needless to say\|at the end of the day\|in today.s fast-paced\|as technology continues to evolve\|it.s worth mentioning\)' || true)
  if [[ $filler -gt 3 ]]; then
    echo "  ⚠ Filler phrases in docs: $filler empty-calorie sentences"
    echo "    → Delete sentences that don't add new information"
    ((WARNINGS++)) || true
  fi

  # 20. Over-hedging
  hedging=$(echo "$MD_DIFF" | grep -ci '^\+.*\(it.s important to\|it should be noted\|one might argue\|arguably\|to some extent\|in some cases\|in many ways\|various factors\)' || true)
  if [[ $hedging -gt 3 ]]; then
    echo "  ⚠ Over-hedging in docs: $hedging wishy-washy qualifiers"
    echo "    → Take a stance. Be direct."
    ((WARNINGS++)) || true
  fi

  # 21. Em-dash overuse (AI signature — tripled on Reddit since LLM adoption)
  # Source: Artificial Ignorance "Em Dash Conspiracy" data
  emdash=$(echo "$MD_DIFF" | grep -c '^\+.*—' || true)
  if [[ $emdash -gt 10 ]]; then
    echo "  ⚠ Em-dash overuse in docs: $emdash lines with — (AI writing signature)"
    echo "    → Use periods or commas. Em-dashes are spice, not staple."
    ((WARNINGS++)) || true
  fi

  # 22. Servile positivity / sycophantic tone
  servile=$(echo "$MD_DIFF" | grep -ci '^\+.*\(game.changer\|powerful tool\|best practices\|world-class\|cutting.edge\|next.level\|elevate\|revolutioniz\|transform\)' || true)
  if [[ $servile -gt 3 ]]; then
    echo "  ⚠ Servile positivity in docs: $servile marketing-speak phrases"
    echo "    → Describe what it does, not how amazing it is"
    ((WARNINGS++)) || true
  fi

  # 23. Snappy triads: three-beat patterns (AI rhetorical cliche)
  # Source: Artificial Ignorance "Snappy triads" observation
  triads=$(echo "$MD_DIFF" | grep -ci '^\+.*\. .*\. .*\.$' || true)
  triad_patterns=$(echo "$MD_DIFF" | grep -ci '^\+.*\(Think bigger\|Act bolder\|Move faster\|Fast, efficient\|Simple, clean\)' || true)
  if [[ $((triads + triad_patterns)) -gt 5 ]]; then
    echo "  ⚠ Snappy triads: $((triads + triad_patterns)) three-beat patterns"
    echo "    → Vary your rhythm. Not everything needs to come in threes."
    ((WARNINGS++)) || true
  fi

  # 24. Unicode formatting (bold/italic via Unicode chars — almost exclusively AI)
  # Source: Artificial Ignorance observation on Unicode formatting
  # Matches Unicode Mathematical Alphanumeric Symbols (U+1D400-1D7FF)
  unicode_fmt=$(echo "$MD_DIFF" | grep -c '^\+.*[𝐀𝐁𝐂𝐃𝐄𝐅𝐆𝐇𝐈𝐉𝐊𝐋𝐌𝐍𝐎𝐏𝐐𝐑𝐒𝐓𝐔𝐕𝐖𝐗𝐘𝐙𝗮𝗯𝗰𝗱𝗲𝗳𝗴𝗵𝗶𝗷𝗸𝗹𝗺𝗻𝗼𝗽𝗾𝗿𝘀𝘁𝘂𝘃𝘄𝘅𝘆𝘇𝘈𝘉𝘊𝘋𝘌𝘍𝘎𝘏𝘐𝘑𝘒𝘓𝘔𝘕𝘖𝘗𝘘𝘙𝘚𝘛𝘜𝘝𝘞𝘟𝘠𝘡]' 2>/dev/null || true)
  if [[ $unicode_fmt -gt 0 ]]; then
    echo "  ⚠ Unicode formatting: $unicode_fmt lines with Unicode bold/italic chars"
    echo "    → Use markdown formatting, not Unicode mathematical symbols"
    ((WARNINGS++)) || true
  fi

  # 25. Emoji bullet patterns in professional docs
  # Source: Artificial Ignorance, remove-ai-slop
  emoji_bullets=$(echo "$MD_DIFF" | grep -c '^\+[[:space:]]*[✅💡📊🚀✨🔥📝🎯⚡📦🛡🔍💪🌟⭐🔑]' 2>/dev/null || true)
  if [[ $emoji_bullets -gt 3 ]]; then
    echo "  ⚠ Emoji bullet points: $emoji_bullets lines with emoji-led bullets"
    echo "    → Use standard markdown bullets (- or *) in technical docs"
    ((WARNINGS++)) || true
  fi
fi

# Summary
echo ""
if [[ $WARNINGS -eq 0 ]]; then
  echo "  ✓ No slop detected — code looks hand-crafted"
else
  echo "  $WARNINGS pattern(s) found — review for AI-generated bloat"
fi

exit 0
