#!/bin/bash
SELF=$$
for p in $(pgrep -f mtjs 2>/dev/null); do
  [ "$p" = "$SELF" ] && continue
  cmd=$(tr "\0" " " < /proc/$p/cmdline 2>/dev/null)
  case "$cmd" in *b_run-nodes*|*keepalive*) kill "$p" 2>/dev/null; echo "killed $p";; esac
done
