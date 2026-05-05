#!/usr/bin/env bash
#
# docker-serve.sh — Manage Docker Compose services (up/down/status).
#
# COMMANDS:
#   up      Start all services (SearXNG)
#   down    Stop all services
#   status  Show running containers
#   help    Show this help
#
# EXAMPLES:
#   scripts/dev/docker-serve.sh up
#   scripts/dev/docker-serve.sh down
#   scripts/dev/docker-serve.sh status

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

COMPOSE_FILE=".config/docker-compose.yml"

# Use sudo on Linux if user is not in docker group
DOCKER="docker"
if [[ "$(uname -s)" != "Darwin" ]] && ! docker info >/dev/null 2>&1; then
  DOCKER="sudo docker"
fi

case "${1:-help}" in
up)
  $DOCKER compose -f "${COMPOSE_FILE}" up -d
  echo "✓ Services started"
  $DOCKER compose -f "${COMPOSE_FILE}" ps
  ;;
down)
  $DOCKER compose -f "${COMPOSE_FILE}" down
  echo "✓ Services stopped"
  ;;
status)
  $DOCKER compose -f "${COMPOSE_FILE}" ps
  ;;
help | --help | -h)
  echo "Usage: docker-serve.sh <up|down|status>"
  echo ""
  echo "  up      Start services (SearXNG)"
  echo "  down    Stop services"
  echo "  status  Show running containers"
  ;;
*)
  echo "Unknown command: $1" >&2
  exit 1
  ;;
esac
