#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# Big Brother AI Simulator -- Acceptance Test Runner
#
# Runs all three acceptance test suites and reports overall PASS/FAIL.
# Usage:  bash tests/run-acceptance.sh
# ─────────────────────────────────────────────────────────────────────────────

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Track results
TOTAL_SUITES=0
FAILED_SUITES=0
SUITE_RESULTS=()

run_suite() {
  local name="$1"
  local file="$2"
  TOTAL_SUITES=$((TOTAL_SUITES + 1))

  echo ""
  echo "######################################################################"
  echo "# Running: $name"
  echo "######################################################################"
  echo ""

  if npx ts-node --project "$SCRIPT_DIR/tsconfig.json" "$file" 2>&1; then
    SUITE_RESULTS+=("PASS: $name")
  else
    SUITE_RESULTS+=("FAIL: $name")
    FAILED_SUITES=$((FAILED_SUITES + 1))
  fi
}

# ── Ensure dependencies are available ────────────────────────────────────────

# Check if ts-node is available; install locally if not
if ! npx ts-node --version > /dev/null 2>&1; then
  echo "Installing ts-node and typescript..."
  cd "$PROJECT_ROOT"
  npm install --no-save ts-node typescript @types/node 2>&1
fi

# ── Run test suites ──────────────────────────────────────────────────────────

cd "$PROJECT_ROOT"

run_suite "Demo A -- Web Control Room Acceptance"     "$SCRIPT_DIR/demo-a-acceptance.test.ts"
run_suite "Demo B -- Desktop Prestige House Acceptance" "$SCRIPT_DIR/demo-b-acceptance.test.ts"
run_suite "Shared Data Contract"                       "$SCRIPT_DIR/shared-contract.test.ts"

# ── Report ───────────────────────────────────────────────────────────────────

echo ""
echo "######################################################################"
echo "# ACCEPTANCE TEST SUMMARY"
echo "######################################################################"
echo ""

for result in "${SUITE_RESULTS[@]}"; do
  echo "  $result"
done

echo ""
PASSED_SUITES=$((TOTAL_SUITES - FAILED_SUITES))
echo "  Suites: $PASSED_SUITES/$TOTAL_SUITES passed"
echo ""

if [ "$FAILED_SUITES" -gt 0 ]; then
  echo "  OVERALL: FAIL"
  echo ""
  exit 1
else
  echo "  OVERALL: PASS"
  echo ""
  exit 0
fi
