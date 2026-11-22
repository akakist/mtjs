#!/bin/bash
set -u
SELF=$$
MTJS="$HOME/mtjs/src/build/bin/mtjs"

for p in $(pgrep -f 'mtjs' 2>/dev/null); do
    [ "$p" = "$SELF" ] && continue
    cmd=$(tr '\0' ' ' < /proc/$p/cmdline 2>/dev/null)
    case "$cmd" in
        *b_run-nodes*|*keepalive*|*chk-*) kill "$p" 2>/dev/null;;
    esac
done
sleep 2

cd /tmp/regress || exit 1
rm -rf db db0 db1 db2 db3 db4 db5 db6 db7 db8 db9 storage
rsync -a --exclude 'db*' --exclude storage --exclude '*.log' \
      "$HOME/mtjs/src/js/ts/dist/" /tmp/regress/
nohup "$MTJS" b_run-nodes.js > /tmp/regress/cluster.log 2>&1 &
echo "started pid $! bin=$MTJS"
sleep 15
echo "ports: $(ss -ltn 2>/dev/null | grep -c ':23[0-9][0-9]')"
echo "blocks: $(grep -c 'do_start_block' /tmp/regress/cluster.log 2>/dev/null)"
