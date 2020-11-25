#!/bin/sh

## Make file
gcc -o client client.cpp

## Run files
./client -i 127.0.0.1 -p 10001
