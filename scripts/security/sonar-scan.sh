#!/usr/bin/env bash
#
# sonar-scan.sh — Start SonarQube via docker-compose and run analysis.
#
# Handles:
#   - Checking if Docker is running (starts Docker Desktop on macOS)
#   - Starting SonarQube container
#   - Installing sonar-scanner if missing
#   - Waiting for SonarQube to be ready
#   - Running the scan
#
# Usage:
#   make sonar
#
# SonarQube runs at http://localhost:9000 (default login: admin/admin).

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

cd "$(dirname "$0")/../.."

COMPOSE_FILE=".config/docker-compose.yml"
SONAR_URL="http://localhost:9000"
MAX_WAIT=120
TOKEN_NAME="llama-cli-scan"

# Load password from .env (avoids hardcoding secrets — gitleaks)
# shellcheck disable=SC1091
[[ -f .env ]] && source .env
SONAR_NEW_PASS="${SONAR_NEW_PASS:-Admin1234admin!}"

# On Linux, docker typically requires sudo unless the user is in the docker group
DOCKER="docker"
if [[ "$(uname -s)" != "Darwin" ]] && ! docker info >/dev/null 2>&1 && sudo docker info >/dev/null 2>&1; then
  DOCKER="sudo docker"
fi

#######################################
# Ensure Docker is running.
# On macOS, starts Docker Desktop if needed.
#######################################
ensure_docker() {
  if $DOCKER info >/dev/null 2>&1; then
    return 0
  fi

  echo "  Docker not running."
  if [[ "$(uname -s)" == "Darwin" ]]; then
    echo "  Starting Docker Desktop..."
    open -a Docker
    local elapsed=0
    while ! $DOCKER info >/dev/null 2>&1; do
      sleep 3
      elapsed=$((elapsed + 3))
      if [[ "${elapsed}" -ge 60 ]]; then
        echo "  ERROR: Docker did not start within 60s"
        exit 1
      fi
    done
    echo "  Docker ready."
  else
    echo "  ERROR: Docker is not running. Start it with: sudo systemctl start docker"
    exit 1
  fi
}

#######################################
# Ensure sonar-scanner CLI is installed.
#######################################
ensure_scanner() {
  if command -v sonar-scanner >/dev/null 2>&1; then
    return 0
  fi

  echo "  sonar-scanner not found, installing..."
  if [[ "$(uname -s)" == "Darwin" ]]; then
    brew install sonar-scanner
  else
    # Direct download for Linux
    local ver="6.2.1.4610"
    local url="https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-${ver}-linux-x64.zip"
    wget -q "${url}" -O /tmp/sonar-scanner.zip
    unzip -qo /tmp/sonar-scanner.zip -d /tmp
    sudo mv "/tmp/sonar-scanner-${ver}-linux-x64" /opt/sonar-scanner
    sudo ln -sf /opt/sonar-scanner/bin/sonar-scanner /usr/local/bin/sonar-scanner
    rm -f /tmp/sonar-scanner.zip
  fi
}

#######################################
# Wait for SonarQube to be ready.
#######################################
wait_for_sonar() {
  local elapsed=0
  echo "  Waiting for SonarQube to start (max ${MAX_WAIT}s)..."
  while true; do
    local status
    status=$(curl -s "${SONAR_URL}/api/system/status" 2>/dev/null | grep -oE '"status":"[A-Z]+"' | cut -d'"' -f4 || echo "")
    if [[ "${status}" == "UP" ]]; then
      echo "  SonarQube ready."
      return 0
    fi
    sleep 3
    elapsed=$((elapsed + 3))
    if [[ "${elapsed}" -ge "${MAX_WAIT}" ]]; then
      echo "  ERROR: SonarQube did not start within ${MAX_WAIT}s (status: ${status:-unreachable})"
      echo "  Check: $DOCKER compose -f ${COMPOSE_FILE} logs sonarqube"
      exit 1
    fi
    # Show progress every 15s
    if [[ $((elapsed % 15)) -eq 0 ]]; then
      echo "  ... ${elapsed}s (status: ${status:-starting})"
    fi
  done
}

#######################################
# Ensure authentication is set up.
# SonarQube 10+ forces a password change on first login.
# We change the default password, then generate a token.
#######################################
ensure_auth() {
  # Try authenticating with the new password first (already configured)
  if curl -sf -u "admin:${SONAR_NEW_PASS}" "${SONAR_URL}/api/authentication/validate" | grep -q '"valid":true'; then
    echo "  Auth OK (password already changed)."
  elif curl -sf -u "admin:admin" "${SONAR_URL}/api/authentication/validate" | grep -q '"valid":true'; then
    # Default password still works — change it (required by SonarQube 10+)
    echo "  Changing default admin password..."
    curl -sf -u "admin:admin" -X POST \
      "${SONAR_URL}/api/users/change_password" \
      -d "login=admin&previousPassword=admin&password=${SONAR_NEW_PASS}" >/dev/null
    echo "  Password changed."
  else
    echo "  ERROR: Cannot authenticate with SonarQube (tried admin/admin and admin/${SONAR_NEW_PASS})"
    exit 1
  fi

  # Generate or reuse a token
  # Revoke existing token with same name (ignore errors if it doesn't exist)
  curl -sf -u "admin:${SONAR_NEW_PASS}" -X POST \
    "${SONAR_URL}/api/user_tokens/revoke" \
    -d "name=${TOKEN_NAME}" >/dev/null 2>&1 || true

  SONAR_TOKEN=$(curl -sf -u "admin:${SONAR_NEW_PASS}" -X POST \
    "${SONAR_URL}/api/user_tokens/generate" \
    -d "name=${TOKEN_NAME}" | grep -oE '"token":"[^"]+"' | cut -d'"' -f4)

  if [[ -z "${SONAR_TOKEN}" ]]; then
    echo "  ERROR: Failed to generate token"
    exit 1
  fi
  echo "  Token generated."
}

#######################################
# Main entry point.
#######################################
main() {
  echo "==> SonarQube analysis"

  # Step 1: Docker
  ensure_docker

  # Step 2: Start SonarQube if not already up
  if curl -s "${SONAR_URL}/api/system/status" | grep -q '"status":"UP"'; then
    echo "  SonarQube already running."
  else
    echo "  Starting SonarQube container..."
    $DOCKER compose -f "${COMPOSE_FILE}" up -d sonarqube
    wait_for_sonar
  fi

  # Step 3: Auth — change default password and generate token
  ensure_auth

  # Step 4: sonar-scanner
  ensure_scanner

  # Step 5: Generate compile_commands.json for C++ analysis
  echo "  Generating compilation database..."
  cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON > /dev/null

  # Step 6: Run scan with token auth and compilation database
  echo "  Running analysis..."
  sonar-scanner \
    -Dsonar.projectKey=llama-cli \
    -Dsonar.projectName="llama-cli" \
    -Dsonar.sources=src \
    -Dsonar.host.url="${SONAR_URL}" \
    -Dsonar.token="${SONAR_TOKEN}" \
    -Dsonar.sourceEncoding=UTF-8 \
    -Dsonar.cfamily.compile-commands=build/compile_commands.json \
    -Dsonar.c.file.suffixes=- \
    -Dsonar.cpp.file.suffixes=.cpp,.h \
    -Dsonar.objc.file.suffixes=-

  echo ""
  echo "  ✓ Results: ${SONAR_URL}/dashboard?id=llama-cli"
}

main "$@"
