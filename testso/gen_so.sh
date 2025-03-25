#!/bin/bash

g++ -c add.cpp -fPIC
g++ -shared add.o -o libadd.so
g++ main.cpp -o main -I./ -L./ -ladd -Wl,-rpath=./