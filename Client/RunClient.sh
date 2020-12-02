#!/bin/sh

## Make file
g++ -o client client.cpp

## Run files
./client -i 127.0.0.1 -p 10001
