#!/usr/bin/env bash
set -euo pipefail

# Demo: simulate a market stream and run the parser against it.
# Usage: scripts/demo_stream.sh [PORT] [REPEAT]
# PORT: UDP port to listen on (default 9001)
# REPEAT: how many messages to send (default 1000)

PORT=${1:-9001}
REPEAT=${2:-1000}

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN_DIR="$ROOT/bin"
mkdir -p "$BIN_DIR"

echo "Building recv_parse..."
gcc -O2 -Wall -std=c11 -o "$BIN_DIR/recv_parse" "$ROOT/src/recv_parse.c"

PAYLOAD="$ROOT/payload.bin"

echo "Preparing payload ($REPEAT messages)..."
# message bytes from cfiles/raw/message_a.cpp
MSG_HEX="61 00 00 13 F8 F6 49 74 92 00 00 00 00 B2 D0 5E 08 53 00 02 13 45 00 05 00 08"

# create one message binary
python3 - <<PY > /dev/null
import sys
msg = bytes.fromhex('$MSG_HEX')
rep = $REPEAT
with open('$PAYLOAD', 'wb') as f:
    for _ in range(rep):
        f.write(msg)
sys.stderr.write('wrote %d bytes to %s\n' % (len(msg)*rep, '$PAYLOAD'))
PY

echo "Starting fast parser (recv_parse_fast) on port $PORT"
"$BIN_DIR/recv_parse_fast" "$PORT" &
PID=$!

sleep 0.5

echo "Streaming payload to 127.0.0.1:$PORT using udpsend_stream_file"
"$ROOT/bin/udpsend_stream_file" 127.0.0.1 "$PORT" "$PAYLOAD" 26 0

echo "waiting 1s for receiver output..."
sleep 1

echo "killing parser (pid $PID)"
kill "$PID" 2>/dev/null || true

echo "demo complete"
