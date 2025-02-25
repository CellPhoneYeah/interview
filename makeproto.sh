#!/bin/bash
outpath="./proto/generated"
if [ ! -d "${outpath}" ]; then
    mkdir -p ${outpath}
fi
rm -rf ${outpath}/*
protoc --cpp_out=${outpath} ./proto/*.proto