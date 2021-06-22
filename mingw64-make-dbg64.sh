#!/usr/bin/bash
_file=$(readlink -f $0)
_cdir=$(dirname $_file)
_name=$(basename $_file)

echo ${_cdir}

cd ${_cdir} && make RELEASE=0 BITS=64