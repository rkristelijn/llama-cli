#!/usr/bin/env bash
#
# setup-remote.sh — One-liner setup for llama-cli on a fresh machine.
#
# Installs Ollama, pulls a default model, installs llama-cli binary,
# and generates a working .env configuration.
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/setup-remote.sh | bash
#   MODEL=gemma4:26b bash setup-remote.sh
#
# Environment:
#   MODEL       — Model to pull (default: gemma4:e4b, small and fast)
#   INSTALL_DIR — Where to install llama-cli (default: /usr/local/bin)
#   SKIP_OLLAMA — Set to 1 to skip Ollama installation
#
# Requirements:
#   - curl
#   - Linux x64/arm64 or macOS arm64

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

MODEL="${MODEL:-gemma4:e4b}"
INSTALL_DIR="${INSTALL_DIR:-/usr/local/bin}"
SKIP_OLLAMA="${SKIP_OLLAMA:-0}"

#######################################
# Print a styled message.
# Arguments:
#   $1 — message
#######################################
info() { echo "  → $1"; }
ok() { echo "  ✓ $1"; }
die() { echo "  ✗ ERROR: $1" >&2; exit 1; }

#######################################
# Install Ollama if not present.
#######################################
install_ollama() {
  if [[ "$SKIP_OLLAMA" == "1" ]]; then
    info "Skipping Ollama install (SKIP_OLLAMA=1)"
    return
  fi

  if command -v ollama &>/dev/null; then
    ok "Ollama already installed ($(ollama --version 2>/dev/null || echo 'unknown'))"
    return
  fi

  info "Installing Ollama..."
  curl -fsSL https://ollama.com/install.sh | sh
  ok "Ollama installed"
}

#######################################
# Start Ollama if not running.
#######################################
start_ollama() {
  if curl -s --max-time 3 http://localhost:11434/api/tags &>/dev/null; then
    ok "Ollama is running"
    return
  fi

  info "Starting Ollama..."
  if command -v systemctl &>/dev/null && systemctl is-enabled ollama &>/dev/null 2>&1; then
    sudo systemctl start ollama
  else
    # Start in background for non-systemd systems
    ollama serve &>/dev/null &
    sleep 3
  fi

  # Verify it started
  if curl -s --max-time 5 http://localhost:11434/api/tags &>/dev/null; then
    ok "Ollama started"
  else
    die "Failed to start Ollama. Try: ollama serve"
  fi
}

#######################################
# Pull the default model.
#######################################
pull_model() {
  info "Pulling model: $MODEL (this may take a while)..."
  ollama pull "$MODEL"
  ok "Model $MODEL ready"
}

#######################################
# Install llama-cli binary.
#######################################
install_llama_cli() {
  if command -v llama-cli &>/dev/null; then
    ok "llama-cli already installed ($(llama-cli --version 2>/dev/null | head -1))"
    return
  fi

  info "Installing llama-cli..."
  curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/install.sh | bash # nosemgrep
  ok "llama-cli installed to $INSTALL_DIR"
}

#######################################
# Generate .env if not present.
#######################################
generate_env() {
  local env_path="$HOME/.llama-cli/.env"
  mkdir -p "$HOME/.llama-cli"

  if [[ -f "$env_path" ]]; then
    ok ".env already exists at $env_path"
    return
  fi

  cat > "$env_path" <<EOF
LLAMA_PROVIDER=ollama
OLLAMA_HOST=localhost
OLLAMA_PORT=11434
OLLAMA_MODEL=$MODEL
EOF
  ok "Generated $env_path"
}

#######################################
# Main entry point.
#######################################
main() {
  echo ""
  echo "  llama-cli remote setup"
  echo "  ─────────────────────"
  echo ""

  install_ollama
  start_ollama
  pull_model
  install_llama_cli
  generate_env

  echo ""
  echo "  ✓ Setup complete! Run: llama-cli"
  echo ""
  echo "  Model: $MODEL"
  echo "  Config: $HOME/.llama-cli/.env"
  echo ""
}

main "$@"
