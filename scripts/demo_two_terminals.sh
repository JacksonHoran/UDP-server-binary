#!/usr/bin/env bash
set -euo pipefail

# Open two terminals and run the demo receiver + sender.
# Usage: scripts/demo_two_terminals.sh [PORT] [REPEAT]
# - PORT: UDP port to use (default 9001)
# - REPEAT: number of messages to repeat in payload (default 1000)

PORT=${1:-9001}
REPEAT=${2:-1000}

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN_DIR="$ROOT/bin"
PAYLOAD="$ROOT/payload.bin"

echo "Building recv_parse..."
gcc -O2 -Wall -std=c11 -o "$BIN_DIR/recv_parse" "$ROOT/src/recv_parse.c"

echo "Preparing payload ($REPEAT messages)..."
MSG_HEX="61 00 00 13 F8 F6 49 74 92 00 00 00 00 B2 D0 5E 08 53 00 02 13 45 00 05 00 08"

python3 - <<PY > /dev/null
import sys
msg = bytes.fromhex('$MSG_HEX')
rep = $REPEAT
with open('$PAYLOAD', 'wb') as f:
    for _ in range(rep):
        f.write(msg)
sys.stderr.write('wrote %d bytes to %s\n' % (len(msg)*rep, '$PAYLOAD'))
PY

CMD_RECV="cd '$ROOT' && ./bin/recv_parse_fast $PORT"
CMD_SEND="cd '$ROOT' && ./bin/udpsend_stream_file 127.0.0.1 $PORT '$PAYLOAD' 26 0"

if command -v tmux >/dev/null 2>&1; then
    session="udp-demo-$$"
    echo "tmux found: creating session '$session'"
    tmux new-session -d -s "$session" "$CMD_RECV"
    tmux split-window -h -t "$session" "$CMD_SEND"
    tmux select-layout -t "$session" tiled
    echo "Attaching to tmux session. Press Ctrl-b d to detach. To kill: tmux kill-session -t $session"
    tmux attach -t "$session"
    exit 0
fi

# Fallback for macOS Terminal.app using AppleScript (opens two windows)
if command -v osascript >/dev/null 2>&1; then
    echo "tmux not found; using Terminal.app via osascript"
    # Open first window running receiver
    osascript <<APPLESCRIPT
tell application "Terminal"
    activate
    do script "${CMD_RECV}"
    delay 0.5
    do script "${CMD_SEND}"
end tell
APPLESCRIPT
    echo "Opened two Terminal windows (or tabs) running receiver and sender."
    exit 0
fi

echo "No tmux or osascript available. I can run in the background instead."
echo "Receiver will run in background (stdout/stderr to recv.log). Sender will run foreground."
"$CMD_RECV" &> "$ROOT/recv.log" &
sleep 0.5
"$CMD_SEND"
