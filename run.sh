#!/bin/bash
export LLVM_DIR=/usr/bin/llvm-3.8/share/llvm/cmake
HOME=`pwd`
mkdir -p instrumentor/build/
cd instrumentor/build/
rm -rf *
cmake -v ..
make VERBOSE=1
cd ..
rm *.o 
BENCHS=/home/hmatar/Desktop/VirtualBoxShare/ADFinspec/benchmarks

clang++ -Xclang -load -Xclang build/libADFInstrumentationPass.so -c -Wall -std=c++11 test.cpp

#clang++ -Xclang -load -Xclang build/libADFInstrumentationPass.so -c -Wall -O1 -Wno-unused-but-set-variable -I${BENCHS}/src -I${BENCHS}/include -I${BENCHS}/include/stm_stl  -DADF_STM -DADF -std=c++11 -pthread  -I${BENCHS}/include/atomic_ops -I${BENCHS}/tmmisc ${BENCHS}/dwarfs/sparse_algebra/sparse_algebra_adf.cpp
#exit
echo "COMPLETE 1st compile step"

g++ -c rtlib.c
echo "COMPLETE 2nd compile step"

#clang++ -o sparse_algebra_adf sparse_algebra_adf.o rtlib.o -L/home/hmatar/Desktop/VirtualBoxShare/ADFinspec/benchmarks/lib -litm -ladf -lpthread -lsfftw
echo "COMPLETE 3rd compile step"
g++ -o  sparse_algebra_adf rtlib.o test.o
echo "RUNNING"

./sparse_algebra_adf
