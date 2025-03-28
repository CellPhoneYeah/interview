#!/bin/bash
if [ -e "net/bin/server" ]; then
    ./ellnet/bin/server
else
    echo "file not exists"
fi