#!/usr/bin/env bash

# Simply running the script will either spin up a new VM,
# or if the VM is already running it will run a provision
# 
# if the script is called and the word destroy is passed in 
# the VM will still be spun up or provisioned, but it will
# also be forcefully destroyed.

# Where to dump the ansible log file.  
export ANSIBLE_LOG_PATH=/tmp/ansible_$(date +%Y%m%d%H%M%S).log

# if the vm is not running then run it
# else the vm is running simply reprovision (run ansible)

if [ "$1" == "-g" ]; then
	git add -A
	git commit -m "test for bits.c"
	git push
fi

vagrant up --provision

cat ./tf/tmp/DriverLog.txt

if [ "$2" == "-e" ]; then
	cat ./tf/tmp/ErrorLog.txt
fi

rm -rf ./tf
# 
if [ "$1" == "destroy" ]; then
    vagrant destroy -f
# else
#   echo "try again"
fi

