#!/bin/bash
set -u
/tmp/regress/stop_nodes.sh >/dev/null 2>&1
sleep 2
rm -rf /tmp/regress/db5
cd /tmp/regress || exit 1
nohup "$HOME/mtjs/src/build/bin/mtjs" b_run-nodes.js > /tmp/regress/cluster2.log 2>&1 &
echo "started pid $!"
sleep 20
echo "blocks_20s: $(grep -c do_start_block /tmp/regress/cluster2.log)"
echo "=== n5 / sync ==="
grep -E "\[n5\].*(sync|genesis|GENESIS)|request sync|sync: done|not matched|rebuild" /tmp/regress/cluster2.log | head -15
sleep 10
echo "blocks_30s: $(grep -c do_start_block /tmp/regress/cluster2.log)"
