#!/bin/bash

gcc -std=c99 main.c -o main -lncursesw

if [ $1 ]; then
	if [ $1 == "-x" ]; then
		./main
	fi
fi