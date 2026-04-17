#!/usr/bin/env bash
#
# pre-push — Run comprehensive checks before pushing (lints already done in pre-commit).
#
# If code changed: runs analysis, tests, security.
# If only docs: verifies INDEX.md is up-to-date.
#
# Installed by: make hooks / make setup
# Location:     .git/hooks/pre-push (symlink or copy from scripts/git/pre-push.sh)
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail

exec bash scripts/git/prepush-check.sh
