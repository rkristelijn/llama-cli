#!/usr/bin/env bash
#
# install.sh — Download and install llama-cli from GitHub Releases.
#
# Detects OS and architecture, downloads the correct binary, and installs
# to /usr/local/bin (or a custom directory via INSTALL_DIR).
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/install.sh | bash
#   INSTALL_DIR=~/.local/bin bash install.sh
#
# Environment:
#   INSTALL_DIR — Override install directory (default: /usr/local/bin)
#   VERSION     — Install a specific version (default: latest)

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

REPO="rkristelijn/llama-cli"
INSTALL_DIR="${INSTALL_DIR:-/usr/local/bin}"

#######################################
# Print an error message and exit.
# Arguments:
#   $1 — error message
#######################################
die() {
  echo "ERROR: $1" >&2
  exit 1
}

#######################################
# Detect OS and architecture, set BINARY name.
# Sets globals: BINARY
#######################################
detect_platform() {
  local os arch
  os="$(uname -s)"
  arch="$(uname -m)"

  case "$os" in
    Linux)
      case "$arch" in
        x86_64)       BINARY="llama-cli-linux-x64" ;;
        aarch64|arm64) BINARY="llama-cli-linux-arm64" ;;
        *) die "Unsupported Linux architecture: $arch" ;;
      esac
      ;;
    Darwin)
      case "$arch" in
        arm64) BINARY="llama-cli-macos-arm64" ;;
        *) die "Unsupported macOS architecture: $arch (only Apple Silicon is supported)" ;;
      esac
      ;;
    *) die "Unsupported OS: $os" ;;
  esac
}

#######################################
# Resolve the download URL for the release binary.
# Sets globals: URL, RESOLVED_VERSION
#######################################
resolve_url() {
  if [[ -n "${VERSION-}" ]]; then
    RESOLVED_VERSION="$VERSION"
    URL="https://github.com/$REPO/releases/download/v${VERSION}/${BINARY}"
  else
    URL="https://github.com/$REPO/releases/latest/download/${BINARY}"
    RESOLVED_VERSION="latest"
  fi
}

#######################################
# Download and install the binary.
#######################################
install_binary() {
  local tmpfile
  tmpfile="$(mktemp)"
  trap 'rm -f "$tmpfile"' EXIT

  echo "Downloading llama-cli ($RESOLVED_VERSION) for $(uname -s)/$(uname -m)..."

  if command -v curl >/dev/null 2>&1; then
    curl -fSL --progress-bar "$URL" -o "$tmpfile"
  elif command -v wget >/dev/null 2>&1; then
    wget -q --show-progress "$URL" -O "$tmpfile"
  else
    die "Neither curl nor wget found. Install one and try again."
  fi

  chmod +x "$tmpfile"

  # Install — use sudo if needed
  if [[ -w "$INSTALL_DIR" ]]; then
    mv "$tmpfile" "$INSTALL_DIR/llama-cli"
  else
    echo "Installing to $INSTALL_DIR (requires sudo)..."
    sudo mv "$tmpfile" "$INSTALL_DIR/llama-cli"
  fi

  echo ""
  echo "✓ llama-cli installed to $INSTALL_DIR/llama-cli"
  echo ""
  echo "Get started:"
  echo "  llama-cli              # interactive chat"
  echo "  llama-cli --help       # see all options"
  echo ""
  echo "Requires Ollama running locally: https://ollama.com"
}

main() {
  detect_platform
  resolve_url
  install_binary
}

main "$@"
