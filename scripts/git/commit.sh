#!/usr/bin/env bash
# commit.sh — Interactive conventional commit (ctrl-c to abort)
# Usage: make commit
# @see docs/adr/adr-121-cpm-quality-layer.md

set -o errexit
set -o nounset
set -o pipefail
source lib/cpm/shell/init.sh 2>/dev/null || true

# Analyze staged files
STAGED=$(git diff --cached --name-only 2>/dev/null)
HAS_SRC=$(echo "$STAGED" | grep -c '^src/' || true)
HAS_TEST=$(echo "$STAGED" | grep -c '_test\|test_\|\.test\.' || true)
HAS_DOCS=$(echo "$STAGED" | grep -c '\.md$\|docs/' || true)
STAGED_COUNT=$(echo "$STAGED" | grep -c '.' || true)

# Show staged files + warnings
echo ""
echo "  Staged ($STAGED_COUNT files):  [ctrl-c to abort]"
echo "$STAGED" | head -8 | sed 's/^/    /'
((STAGED_COUNT > 8)) && echo "    ... +$((STAGED_COUNT - 8)) more"
echo ""

# Warnings
if ((HAS_SRC > 0 && HAS_TEST == 0)); then
  echo "  ⚠ Code changed but no tests staged"
fi
if ((HAS_SRC > 0 && HAS_DOCS == 0)); then
  echo "  ⚠ Code changed but no docs staged"
fi
((HAS_SRC > 0 && (HAS_TEST == 0 || HAS_DOCS == 0))) && echo ""

if ((STAGED_COUNT == 0)); then
  echo "  Nothing staged. Use: git add <files>"
  exit 1
fi

# Type
echo "  f)fix a)feat r)refactor d)docs"
echo "  t)test b)build c)ci p)perf s)style"
printf "  Type [f]: "
read -r k
case "${k:-f}" in
f) T=fix ;; a) T=feat ;; r) T=refactor ;; d) T=docs ;;
t) T=test ;; b) T=build ;; c) T=ci ;; p) T=perf ;;
s) T=style ;; x) T=chore ;; *) T=fix ;;
esac

# Scope
printf "  Scope (enter=none): "
read -r S

# Description
echo "  Imperative: add X, fix Y, remove Z"
printf "  Desc: "
read -r D
[[ -z "$D" ]] && {
  echo "  Required."
  exit 1
}

# Breaking?
printf "  Breaking? [y/N]: "
read -r B
BP=""
[[ "$B" =~ ^[yY] ]] && BP="!"

# Build + preview
[[ -n "$S" ]] && LINE="${T}${BP}(${S}): ${D}" || LINE="${T}${BP}: ${D}"
echo ""
echo "  → $LINE"
printf "  Commit? [Y/n]: "
read -r C
[[ "$C" =~ ^[nN] ]] && {
  echo "  Aborted."
  exit 0
}

git commit -m "$LINE"
