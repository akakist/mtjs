#!/bin/bash
set -u
SELF=$$
MTJS="$HOME/mtjs/src/build/bin/mtjs"
# убить только наши нод-процессы (b_run-node / b_run-nodes / keepalive)
for p in $(pgrep -f "$MTJS" 2>/dev/null); do
    [ "$p" = "$SELF" ] && continue
    cmd=$(tr "\0" " " < /proc/$p/cmdline 2>/dev/null)
    case "$cmd" in
        *b_run-node*|*b_run-nodes*|*keepalive*) kill "$p" 2>/dev/null;;
    esac
done
sleep 2
cd /tmp/regress || exit 1
rm -rf db db0 db1 db2 db3 db4 db5 db6 db7 db8 db9 storage node_pids.txt node_*.log
rsync -a --exclude "db*" --exclude storage --exclude "*.log" "$HOME/mtjs/src/js/ts/dist/" /tmp/regress/
for i in $(seq 0 9); do
    MTJS_NODE_IDX=$i MTJS_KEYS_PATH=keys_test.json nohup "$MTJS" b_run-node.js > node_$i.log 2>&1 &
    echo $! >> node_pids.txt
done
sleep 15
echo "pids:"; cat node_pids.txt
echo "ports: $(ss -ltn 2>/dev/null | grep -c ":23[0-9][0-9]")"
