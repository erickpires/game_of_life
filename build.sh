#!/bin/bash

gcc -o3 -std=c99 main.c -o main -Wall -lncursesw

if [ $1 ]; then
	if [ $1 == "-x" ]; then
		./main
	fi
fi