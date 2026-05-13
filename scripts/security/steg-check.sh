#!/usr/bin/env bash
#
# steg-check.sh — Scan image files for steganography using zsteg.
# lint-exempt: max-exits (multiple exit states: error, skip, success)
#
# Usage:
#   bash scripts/security/steg-check.sh [dir]
#
# Returns 1 if suspicious data is found, 0 otherwise.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Navigate to project root
cd "$(dirname "$0")/../.."

TARGET_DIR="${1:-.}"
CONFIG_FILE=".config/zsteg.yml"

# Try to find zsteg in user gem path if not in standard PATH
if ! command -v zsteg >/dev/null 2>&1; then
  USER_GEM_BIN=$(ruby -e 'puts File.join(Gem.user_dir, "bin")' 2>/dev/null || echo "")
  if [[ -n "${USER_GEM_BIN}" && -f "${USER_GEM_BIN}/zsteg" ]]; then
    export PATH="$PATH:${USER_GEM_BIN}"
  fi
fi

if ! command -v zsteg >/dev/null 2>&1; then
  echo "  [skip] zsteg not installed"
  exit 0
fi

# Load ignore list from config
IGNORED_FILES=()
if [[ -f "${CONFIG_FILE}" ]]; then
  # Simple YAML parsing for ignore_files section
  while IFS= read -r line; do
    if [[ "${line}" =~ ^[[:space:]]*-[[:space:]]*\"(.*)\" ]]; then
      IGNORED_FILES+=("${BASH_REMATCH[1]}")
    fi
  done < <(sed -n '/ignore_files:/,/ignore_strings:/p' "${CONFIG_FILE}" | grep ' - "')
fi

echo "==> scanning for steganography (zsteg) in ${TARGET_DIR}..."

# Find PNG and BMP files (supported by zsteg)
# We exclude common directories like .git and build
IMAGES=$(find "${TARGET_DIR}" -type f \( -name "*.png" -o -name "*.bmp" \) \
  -not -path "*/.*" \
  -not -path "*/build/*" \
  -not -path "*/build-fuzz/*" \
  -not -path "*/_deps/*" \
  -not -path "*/node_modules/*")

if [[ -z "${IMAGES}" ]]; then
  echo "  no PNG or BMP files found to scan."
  exit 0
fi

FAILED=0
for img in ${IMAGES}; do
  # Check if file is ignored (using simple substring match for now)
  IS_IGNORED=false
  for ignored in "${IGNORED_FILES[@]}"; do
    if [[ "${img}" == *"${ignored}"* ]]; then
      IS_IGNORED=true
      break
    fi
  done

  if [[ "${IS_IGNORED}" == true ]]; then
    echo "  Skipping ignored file: ${img}"
    continue
  fi

  echo "  Checking ${img}..."
  # zsteg returns details if it finds something.
  RESULT=$(zsteg --all --min-str-len 8 "${img}" 2>&1 || true)

  # Heuristic: if zsteg finds something that looks like a hidden file or large extradata
  if echo "${RESULT}" | grep -Ei "found|extradata|payload" | grep -v "metadata" >/dev/null; then
    echo "  [FAIL] Potential steganography detected in ${img}:"
    echo "${RESULT}" | grep -Ei "found|extradata|payload" | head -n 5
    FAILED=1
  fi
done

if [[ "${FAILED}" -eq 1 ]]; then
  echo "ERROR: Steganography check failed." >&2
  exit 1
fi

echo "  all images passed."
exit 0
