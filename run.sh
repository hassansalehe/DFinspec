#!/bin/bash
HOME=`pwd`
mkdir -p instrumentor/build/
cd instrumentor/build/
cmake ..
make
cd ..
clang -Xclang -load -Xclang build/libADFInstrumentationPass.so -c test.c 
cc -c rtlib.c
cc test.o rtlib.o -o test
./test 
