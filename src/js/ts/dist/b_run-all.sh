#!/bin/bash
# b_run-all.sh — последовательный запуск chk-*.js, exit 1 при любом FAIL

cd "$(dirname "$0")" || exit 1

GREEN='\033[0;32m'
RED='\033[0;31m'
RESET='\033[0m'

TESTS=(
    chk-address.js
    chk-get-user-nonce.js
    chk-mint.js
    chk-rollback.js
    chk-submit-cb.js
    chk-subscribe.js
    chk-subscribe-unsubscribe.js
    chk-state.js
    chk-state1.js
    chk-t14-values.js
    chk-t15-contract-data.js
    chk-t9-node-info.js
    chk-block-failed.js
    chk-block-get.js
    chk-block-happy.js
    chk-block-timeout.js
    chk-caller.js
    chk-delete-granule.js
    chk-ttl.js
    chk-gym-live.js
    chk-gym-buy.js
    chk-gym-emits.js
    chk-gym-rollback.js
    chk-gym-interrupt.js
    chk-gym-2nodes.js
    chk-gym-balance.js
    chk-vote-all.js
)

PASS=0
FAIL=0
FAILED=()

echo "=== B_RUN-ALL ==="
echo "tests: ${#TESTS[@]}"

for t in "${TESTS[@]}"; do
    echo ""
    echo "===== $t ====="
    if [ ! -f "$t" ]; then
        echo -e "${RED}❌ $t — not found${RESET}"
        FAIL=$((FAIL+1))
        FAILED+=("$t — not found")
        continue
    fi
    timeout 120 ./"$t"
    rc=$?
    if [ $rc -eq 0 ]; then
        echo -e "${GREEN}✅ $t OK${RESET}"
        PASS=$((PASS+1))
    else
        echo -e "${RED}❌ $t FAIL (rc=$rc)${RESET}"
        FAIL=$((FAIL+1))
        FAILED+=("$t (rc=$rc)")
    fi
done

echo ""
echo "--- summary ---"
for f in "${FAILED[@]}"; do
    echo "  ❌ $f"
done
echo ""
echo "=== RESULT: $PASS passed, $FAIL failed ==="
if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}DONE${RESET}"
    exit 0
else
    echo -e "${RED}FAIL${RESET}"
    exit 1
fi
