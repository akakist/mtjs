#!/bin/bash
# usage: run_phase.sh <timeout_sec> <test> [test...]
cd /tmp/regress || exit 1
B="$HOME/mtjs/src/build/bin/mtjs"
TO="$1"; shift
PASS=0; FAIL=0; FAILED=()
for t in "$@"; do
    if [ ! -f "$t" ]; then echo "MISSING $t"; FAIL=$((FAIL+1)); FAILED+=("$t missing"); continue; fi
    timeout "$TO" "$B" "$t" > "/tmp/regress/$(basename $t).log" 2>&1
    rc=$?
    if [ $rc -eq 0 ]; then PASS=$((PASS+1)); echo "OK   $t";
    else FAIL=$((FAIL+1)); FAILED+=("$t rc=$rc"); echo "FAIL $t rc=$rc"; fi
done
echo "=== RESULT: PASS=$PASS FAIL=$FAIL ==="
for f in "${FAILED[@]}"; do echo "  FAILED: $f"; done
