#!/bin/sh


printf "Testing pipelining.\n";
(
message="GET / HTTP/1.1\nHost: Pepito\n\n"
printf "$message$message$message$message"
sleep 7
) | telnet 127.0.0.1 8000


