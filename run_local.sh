#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$ROOT"

if [ ! -x bin/restaurant_server ] || [ ! -x bin/student_server ] || \
   [ ! -x bin/student_client ] || [ ! -x bin/restaurant_admin ]; then
    sh ./build.sh
fi

RESTAURANT_LOG="$ROOT/restaurant_server.log"
STUDENT_LOG="$ROOT/student_server.log"
restaurant_pid=""
student_pid=""

cleanup() {
    if [ -n "$student_pid" ]; then kill "$student_pid" 2>/dev/null || true; fi
    if [ -n "$restaurant_pid" ]; then kill "$restaurant_pid" 2>/dev/null || true; fi
    if [ -n "$student_pid" ]; then wait "$student_pid" 2>/dev/null || true; fi
    if [ -n "$restaurant_pid" ]; then wait "$restaurant_pid" 2>/dev/null || true; fi
}
trap cleanup EXIT
trap 'exit 130' INT TERM

./bin/restaurant_server 5001 >"$RESTAURANT_LOG" 2>&1 &
restaurant_pid=$!
sleep 1
if ! kill -0 "$restaurant_pid" 2>/dev/null; then
    echo "Restaurant Server did not start. Check $RESTAURANT_LOG"
    cat "$RESTAURANT_LOG"
    exit 1
fi

./bin/student_server 5002 127.0.0.1 5001 >"$STUDENT_LOG" 2>&1 &
student_pid=$!
sleep 1
if ! kill -0 "$student_pid" 2>/dev/null; then
    echo "Student Server did not start. Check $STUDENT_LOG"
    cat "$STUDENT_LOG"
    exit 1
fi

while true; do
    echo ""
    echo "=== CAFE QUICKORDER ==="
    echo "Both servers are running on this computer."
    echo "1. Open Student Terminal"
    echo "2. Open Restaurant Admin Terminal"
    echo "0. Stop both servers and exit"
    printf "Choice: "
    read -r choice
    case "$choice" in
        1) ./bin/student_client 127.0.0.1 5002 ;;
        2) ./bin/restaurant_admin 127.0.0.1 5001 ;;
        0) break ;;
        *) echo "Please enter 1, 2 or 0." ;;
    esac
done
