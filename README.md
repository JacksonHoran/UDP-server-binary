UDP-server-binary

Purpose
-------
Minimal example tools to send and receive raw binary over UDP. The sender streams a file in fixed-size chunks; the receiver prints the first received packet and exits.

Build
-----
Build all programs with the provided Makefile:

    make

Binaries are produced in `bin/`.

Programs
--------
- `bin/udpsend_stream_file` — stream a file to a remote UDP address. Usage: `udpsend_stream_file <peer_ip> <peer_port> <filename>`.
- `bin/udpsend` — small sender used for testing (see source for options).
- `bin/udprecv` — bind to a UDP port, receive one packet, print sender and payload, then exit. Usage: `udprecv <port>`.

Behavior notes
--------------
- This is an experiment-level codebase: no reliability, no retransmit, no fragmentation handling.
- Use `nc -u -l <port> > file` or `socat` if you want to capture full byte streams with different semantics.


Workflow:
--------------
```
./bin/udprecv 9000
```
This binds to UDP port 9000, receives one packet, prints sender IP:port and the packet payload, then exits.

In another terminal window:
```
./bin/udpsend_stream_file 127.0.0.1 9000 received.bin
```
The sender will print per-packet progress and exit when done.

If you want to capture the full byte stream to disk instead of using udprecv (which exits after one packet), use netcat or socat
```
nc -u -l 9000 > received.bin
```