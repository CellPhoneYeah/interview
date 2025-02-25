#!/bin/bash
if [ -e "build/client" ]; then
    ./build/client
else
    echo "file not exists"
fi