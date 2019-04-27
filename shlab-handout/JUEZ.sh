#!/bin/bash

make clean
make > /dev/null
make rtest04
make test04
