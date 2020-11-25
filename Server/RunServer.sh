#!/bin/sh

## Make file
g++ server.cpp -o server

## Run files
./server -p 10001 -d ./emailFolder
