#!/bin/bash

make clean
make > /dev/null
make rtest06
make test06
