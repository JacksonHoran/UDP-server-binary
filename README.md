# UDP-server-binary

Run make:
```
make
```

Workflow:
--------------
```
./bin/udprecv 9000
```
This binds to UDP port 9000, receives one packet, prints sender IP:port and the packet payload, then exits.

In another terminal window:
```
./bin/udpsend_stream_file 127.0.0.1 9000 test.bin
```
The sender will print per-packet progress and exit when done.

If you want to capture the full byte stream to disk instead of using udprecv (which exits after one packet), use netcat or socat
```
nc -u -l 9000 > received.bin
```

Then, compare each of the files' checksums to verify data:
```
shasum -a 256 test.bin received.bin
1f29a7de578bccc1b883d16c90a0405c18b230e810d497fe9091f3e4836bc596  test.bin
1f29a7de578bccc1b883d16c90a0405c18b230e810d497fe9091f3e4836bc596  received.bin
```
