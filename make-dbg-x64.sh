#!/usr/bin/bash

_file=$(readlink -f $0)
_cdir=$(dirname $_file)
_name=$(basename $_file)

echo "make debug x64 apps"

cd ${_cdir} && make clean && make RELEASE=0 BITS=64 apps