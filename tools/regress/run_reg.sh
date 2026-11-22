#!/bin/bash
cd /tmp/regress
./restart2.sh >/dev/null 2>&1
mkdir -p storage && cp ~/mtjs/src/js/ts/dist/storage/*.js storage/
./run_phase.sh 200 chk-mint.js chk-vote-all.js chk-rollback.js chk-resurrect.js chk-gc.js chk-ttl.js chk-contract-ttl.js gym/chk-gym-rollback.js gym/chk-gym-live.js gym/chk-gym-emits.js gym/chk-gym-buy.js gym/chk-gym-interrupt.js gym/chk-gym-2nodes.js gym/chk-gym-balance.js storage/chk-storage-simple.js storage/chk-watchdog.js storage/chk-gc-address.js storage/chk-gc-contract.js storage/chk-gc-node.js storage/chk-tickets.js storage/chk-storage.js > /tmp/reg_cc.log 2>&1
echo "EXIT=$?" >> /tmp/reg_cc.log
