#!/bin/bash
set -u
/tmp/regress/stop_nodes.sh >/dev/null 2>&1
sleep 2
cd /tmp/regress || exit 1
rm -rf db0 db1 db2 db3 db4 db5 db6 db7 db8 db9
cp b_run-nodes.js b_run-nodes-9.js
sed -i "s/const NODES = 10;/const NODES = 9;/" b_run-nodes-9.js
nohup "$HOME/mtjs/src/build/bin/mtjs" b_run-nodes-9.js > /tmp/regress/deadnode.log 2>&1 &
PID=$!
echo "started pid $PID (9 nodes, n9 dead)"
sleep 20
B1=$(grep -c do_start_block /tmp/regress/deadnode.log)
sleep 15
B2=$(grep -c do_start_block /tmp/regress/deadnode.log)
echo "blocks: $B1 -> $B2"
echo "alive: $(kill -0 $PID 2>/dev/null && echo yes || echo NO)"
echo "=== ошибки ==="
grep -cE "not matched|std::exception|terminate" /tmp/regress/deadnode.log
grep -E "not matched|std::exception|terminate|quorum failed|invalid leader|if\(!cli" /tmp/regress/deadnode.log | tail -8
