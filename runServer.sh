#!/bin/bash
if [ -e "build/server" ]; then
    ./build/server
else
    echo "file not exists"
fi