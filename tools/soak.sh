#!/bin/bash
# soak.sh — длительный стресс-тест ноды (сутки+).
# Нагрузка (тесты по кругу) + периодические мягкие рестарты (краш-восстановление)
# + контроль высот/стола. Остановка: Ctrl+C.
#
# Запуск:   cd /tmp/regress && ./soak.sh [лог-файл]
# Следить:  tail -f /tmp/regress/soak.log
set -u

MTJS="${MTJS:-$HOME/mtjs/src/build/bin/mtjs}"
REG=/tmp/regress
LOG="${1:-$REG/soak.log}"
cd "$REG" || { echo "нет $REG"; exit 1; }

log() { echo "[$(date '+%F %T')] $*" | tee -a "$LOG"; }

# мягкий рестарт: краш + старт БЕЗ wipe (ноды поднимают своё состояние)
soft_restart() {
    ./stop_nodes.sh >/dev/null 2>&1
    sleep 3
    nohup "$MTJS" b_run-nodes.js > "$REG/cluster.log" 2>&1 &
    sleep 15
}

# высоты всех нод одной строкой (n0:H ... n9:H)
heights() {
    timeout 30 "$MTJS" "$REG/chk_heights.js" 2>/dev/null | tail -1
}

# нагрузочные тесты (по кругу, случайный выбор)
TESTS=(
    chk-mint.js chk-vote-all.js chk-rollback.js chk-resurrect.js
    chk-gc.js chk-ttl.js chk-contract-ttl.js chk-callview.js
    gym/chk-gym-buy.js gym/chk-gym-emits.js gym/chk-gym-2nodes.js gym/chk-gym-rollback.js
    gym/chk-gym-live.js gym/chk-gym-interrupt.js gym/chk-gym-balance.js
    storage/chk-storage-simple.js storage/chk-watchdog.js storage/chk-gc-address.js
    storage/chk-gc-contract.js storage/chk-gc-node.js storage/chk-tickets.js storage/chk-storage.js
)

log "=== SOAK START (mtjs=$MTJS) ==="
./restart2.sh >/dev/null 2>&1
sleep 5

round=0
last_blocks=-1
stall_start=""
while true; do
    round=$((round+1))
    blocks=$(grep -c do_start_block "$REG/cluster.log" 2>/dev/null || echo 0)
    hs=$(heights)
    log "round=$round blocks=$blocks heights=[$hs]"

    # ---- стол-детект: 90с без новых блоков → мягкий рестарт ----
    if [ "$blocks" = "$last_blocks" ]; then
        [ -z "$stall_start" ] && stall_start=$(date +%s)
        if [ $(( $(date +%s) - stall_start )) -ge 90 ]; then
            log "!!! STALL (90с, blocks=$blocks) — soft restart"
            soft_restart
            last_blocks=-1; stall_start=""; continue
        fi
    else
        stall_start=""
    fi
    last_blocks=$blocks

    # ---- нагрузка: случайный тест ----
    t=${TESTS[$((RANDOM % ${#TESTS[@]}))]}
    log "  run $t"
    timeout 300 "$MTJS" "$t" >/dev/null 2>&1
    rc=$?
    if [ $rc -ne 0 ]; then log "  $t rc=$rc (не фатально, продолжаем)"; else log "  $t ok"; fi

    # ---- хаос: каждые 10 раундов — мягкий рестарт (краш-восстановление) ----
    if [ $((round % 10)) -eq 0 ]; then
        log "  chaos: soft restart"
        soft_restart
        last_blocks=-1
    fi
    sleep 3
done
