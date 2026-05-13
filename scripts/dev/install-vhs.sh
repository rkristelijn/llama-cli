#!/usr/bin/env bash
# install-vhs.sh — Install VHS (terminal recorder) and dependencies
# Works on Linux (apt/go) and macOS (brew)
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
command_exists() { command -v "$1" &>/dev/null; }

if [[ "$(uname)" == "Darwin" ]]; then
  # macOS
  command_exists brew || {
    echo "Homebrew required"
    exit 1
  }
  brew install charmbracelet/tap/vhs
else
  # Linux
  if command_exists go; then
    go install github.com/charmbracelet/vhs@latest
    echo "Installed vhs to $(go env GOPATH)/bin/vhs"
    echo "Ensure $(go env GOPATH)/bin is in your PATH"
  else
    echo "Go required for VHS install. Install Go first."
    exit 1
  fi
  # Dependencies
  command_exists ffmpeg || sudo apt-get install -y ffmpeg
  command_exists ttyd || sudo apt-get install -y ttyd
fi

echo "VHS installed: $(vhs --version 2>/dev/null || echo 'add go/bin to PATH')"
echo "Record demos: for tape in demos/tapes/*.tape; do vhs \"\$tape\"; done"
