#!/bin/bash
if [ -e "net/bin/server" ]; then
    ./net/bin/server
else
    echo "file not exists"
fi