#!/usr/bin/env bash
#
# syft-sbom.sh — Generate Software Bill of Materials using Syft.
#
# Outputs:
#   1. Syft auto-detected dependencies (GitHub Actions, etc.)
#   2. C++ libraries from CMakeLists.txt (FetchContent)
#   3. Dev tools from .config/versions.env
#
# Usage:
#   bash scripts/security/syft-sbom.sh

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
cd "$(dirname "$0")/../.."

echo "==> generating SBOM (syft + manual deps)..."

if ! command -v syft >/dev/null 2>&1; then
  echo "  [skip] syft not installed"
  exit 0
fi

# 1. Syft auto-detected deps
syft dir:. --exclude='./build/**' --exclude='./build-fuzz/**' --exclude='./build-mull/**' --exclude='./_deps/**' -o table

# 2. C++ libraries (CMake FetchContent)
echo ""
echo "  C++ Libraries (CMakeLists.txt FetchContent):"
awk '/FetchContent_Declare/{getline; gsub(/^[ \t]+/,"",$0); name=$1}
     /GIT_TAG/{printf "    %s @ %s\n", name, $2}' CMakeLists.txt

# 3. Dev tools (versions.env)
echo ""
echo "  Dev Tools (.config/versions.env):"
grep -E "^[A-Z_]+_VERSION=" .config/versions.env | grep -v "^ACTION_" | while IFS='=' read -r key val; do
  tool=$(echo "$key" | sed 's/_VERSION$//' | tr '[:upper:]' '[:lower:]' | tr '_' '-')
  echo "    $tool @ $val"
done

echo ""
echo "  [done] syft"
