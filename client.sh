#!/bin/bash
if [ -e "./net/bin/client" ]; then
    ./net/bin/client
else
    echo "file not exists"
fi