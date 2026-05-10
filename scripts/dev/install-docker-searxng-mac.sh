#!/usr/bin/env bash
# install-docker-searxng-mac.sh — Install Docker and SearXNG for llama-cli web search (macOS)
# Usage: curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/scripts/install-docker-searxng-mac.sh | bash
set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; NC='\033[0m'
info()  { printf "${GREEN}[✓]${NC} %s\n" "$1"; }
warn()  { printf "${YELLOW}[!]${NC} %s\n" "$1"; }
fail()  { printf "${RED}[✗]${NC} %s\n" "$1"; exit 1; }
header(){ printf "\n${GREEN}━━━ %s ━━━${NC}\n" "$1"; }

command_exists() { command -v "$1" &>/dev/null; }

header "Docker + SearXNG Installer (macOS)"
echo "Installs Docker Desktop and runs SearXNG for llama-cli web search."
echo ""

# ─── Homebrew ─────────────────────────────────────────────────────────────────
if ! command_exists brew; then
  warn "Homebrew not found — installing..."
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

# ─── Docker ──────────────────────────────────────────────────────────────────
header "Docker Desktop"
if command_exists docker; then
  info "Docker already installed ($(docker --version))"
else
  brew install --cask docker
  info "Docker Desktop installed — open Docker.app to start the daemon"
  warn "Please start Docker Desktop before continuing."
  echo "  Open Docker from Applications, then re-run this script."
  exit 0
fi

# Wait for Docker daemon
if ! docker info &>/dev/null; then
  warn "Docker daemon not running — trying to start Docker Desktop..."
  open -a Docker
  echo "Waiting for Docker to start (up to 60s)..."
  for i in $(seq 1 30); do
    sleep 2
    docker info &>/dev/null && break
  done
  docker info &>/dev/null || fail "Docker daemon did not start. Open Docker.app manually and re-run."
  info "Docker daemon is running"
fi

# ─── SearXNG ─────────────────────────────────────────────────────────────────
header "SearXNG (local search engine)"
if docker ps --format '{{.Names}}' | grep -q '^searxng$'; then
  info "SearXNG container already running"
elif docker ps -a --format '{{.Names}}' | grep -q '^searxng$'; then
  docker start searxng
  info "SearXNG container restarted"
else
  docker run -d -p 8888:8080 --name searxng --restart unless-stopped searxng/searxng
  info "SearXNG container started on port 8888"
fi

# ─── .env hint ────────────────────────────────────────────────────────────────
header "Done!"
echo ""
echo "Add to your .env (or export in shell):"
echo ""
echo "  ALLOW_WEB_SEARCH=1"
echo "  LLAMA_SEARCH_URL=http://localhost:8888"
echo ""
echo "Optional:"
echo "  LLAMA_SEARCH_LANG=nl-NL"
echo "  LLAMA_SEARCH_LOCATION=Eindhoven, NL"
echo ""
echo "Test: curl -s 'http://localhost:8888/search?q=test&format=json' | head"
