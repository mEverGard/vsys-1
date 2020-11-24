#!/bin/sh

## Make file
gcc -o server server.c

## Run files
./server -p 10001 -d ./emailFolder
