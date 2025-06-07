#!/bin/bash
if [ -e "net/bin/server" ]; then
    ./bin/entrance
else
    echo "file not exists"
fi