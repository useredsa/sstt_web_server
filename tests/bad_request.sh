#!/bin/sh


echo 3;
(
printf "GET  / HTTP/1.1\n\n"
sleep 1
) | telnet 127.0.0.1 8000


