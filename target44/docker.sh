#!/bin/bash
#docker run --rm -it -v $(pwd):/home attack ./docker.sh

if [ $1 == "-o" ]
then

	/usr/bin/objdump -d rtarget > log.txt
elif [ $1 == "-r" ]
then
	./hex2raw < sol.txt > raw.data
	./ctarget < raw.data

elif [ $1 == "-a" ]
then
	gcc -c -o instruction assemble.s
	objdump -d instruction > hex
	rm instruction
fi
