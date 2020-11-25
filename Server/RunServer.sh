#!/bin/sh

## Make file
gcc -o server server.cpp

## Run files
./server -p 10001 -d ./emailFolder
