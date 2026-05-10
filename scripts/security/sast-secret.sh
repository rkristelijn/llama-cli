#!/usr/bin/env bash
# sast-secret.sh — Run gitleaks with version-aware flags.
# Requires gitleaks >= 8.18 for --gitleaks-ignore-path; falls back gracefully.
set -o errexit
set -o nounset
set -o pipefail

if ! command -v gitleaks >/dev/null; then
  echo "  [skip] gitleaks not installed"
  exit 0
fi

GL_VER=$(gitleaks version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+' | head -1 || true)
GL_MAJOR=$(echo "${GL_VER:-0.0}" | cut -d. -f1)
GL_MINOR=$(echo "${GL_VER:-0.0}" | cut -d. -f2)

if [ "${GL_MAJOR:-0}" -gt 8 ] || { [ "${GL_MAJOR:-0}" -eq 8 ] && [ "${GL_MINOR:-0}" -ge 18 ]; }; then
  gitleaks detect --source . --log-level error --no-banner --gitleaks-ignore-path .config/.gitleaksignore
else
  gitleaks detect --source . --log-level error --no-banner
fi
