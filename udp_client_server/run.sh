#!/bin/bash

./build_linux/udp_server &
export APP_PID=$!

( python3 client.py $1 &
python3 client.py $1 &
python3 client.py $1 &
python3 client.py $1 &
wait )

kill $APP_PID
#python3 client.py $1 & python3 client.py $1 & python3 client.py $1 & python3 client.py $1 
