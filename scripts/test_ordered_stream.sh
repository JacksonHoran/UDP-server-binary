#!/usr/bin/env bash
set -euo pipefail

# Generate a payload of 'a' messages with sequential tracking numbers,
# stream it via udpsend_stream_file to a local recv_parse instance,
# and verify that parsed tracking numbers arrive in-order.

PORT=${1:-9002}
REPEAT=${2:-1000}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN_DIR="$ROOT/bin"
PAYLOAD="$ROOT/payload_seq.bin"
PARSED_LOG="$ROOT/parsed_seq.log"

echo "Building recv_parse..."
gcc -O2 -Wall -std=c11 -o "$BIN_DIR/recv_parse" "$ROOT/src/recv_parse.c"

echo "Preparing sequential payload ($REPEAT messages) -> $PAYLOAD"

python3 - <<PY > "$PAYLOAD"
import sys
hex_template = '61 00 00 13 F8 F6 49 74 92 00 00 00 00 B2 D0 5E 08 53 00 02 13 45 00 05 00 08'
base = bytearray.fromhex(hex_template)
rep = int(sys.argv[1]) if len(sys.argv)>1 else $REPEAT
with open('$PAYLOAD', 'wb') as f:
    for i in range(1, rep+1):
        m = bytearray(base)
        # set tracking number at bytes 1-2 (big-endian)
        m[1] = (i >> 8) & 0xFF
        m[2] = i & 0xFF
        f.write(m)
print('wrote', len(m)*rep, 'bytes to', '$PAYLOAD')
PY

echo "Starting recv_parse (port $PORT), logging to $PARSED_LOG"
rm -f "$PARSED_LOG"
"$BIN_DIR/recv_parse" "$PORT" > "$PARSED_LOG" 2>&1 &
PID=$!

sleep 0.5

echo "Streaming payload to 127.0.0.1:$PORT"
"$ROOT/bin/udpsend_stream_file" 127.0.0.1 "$PORT" "$PAYLOAD"

echo "Waiting briefly for receiver to finish..."
sleep 1

kill "$PID" 2>/dev/null || true

echo "Parsing parsed log for tracking numbers"
grep -o 'tracking=[0-9]\+' "$PARSED_LOG" | sed 's/tracking=//' > "$ROOT/received_seq.txt" || true


echo "Comparing expected sequence to received"
python3 - <<PY
import sys
rep = int(${REPEAT})
rec = []
try:
    with open('$ROOT/received_seq.txt') as f:
        for line in f:
            line=line.strip()
            if line:
                rec.append(int(line))
except FileNotFoundError:
    print('No received_seq.txt found; receiver may have produced no parsed messages')
    sys.exit(1)

received_count = len(rec)
missing = len([i for i in range(1, rep+1) if i not in rec])
out_of_order = 0
for i in range(1, received_count):
    if rec[i] < rec[i-1]:
        out_of_order += 1

print('received_count=', received_count)
if missing:
    print('missing_count=', missing)
if out_of_order:
    print('out_of_order_count=', out_of_order)
if not missing and not out_of_order:
    print('OK: sequence in order')

print('\nSample of first 20 received tracking numbers:')
for x in rec[:20]:
    print(x)

PY

echo "done"
