#!/usr/bin/env bash
# install-docker-searxng-linux.sh — Install Docker and SearXNG for llama-cli web search (Linux)
# Usage: curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/scripts/install-docker-searxng-linux.sh | bash
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

header "Docker + SearXNG Installer (Linux)"
echo "Installs Docker Engine and runs SearXNG for llama-cli web search."
echo ""

# ─── Docker ──────────────────────────────────────────────────────────────────
header "Docker Engine"
if command_exists docker; then
  info "Docker already installed ($(docker --version))"
else
  curl -fsSL https://get.docker.com | sh
  sudo usermod -aG docker "$USER"
  info "Docker installed — you may need to log out/in for group changes"
fi

# Start Docker if not running
if ! docker info &>/dev/null; then
  sudo systemctl start docker
  sudo systemctl enable docker
  info "Docker service started and enabled"
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
