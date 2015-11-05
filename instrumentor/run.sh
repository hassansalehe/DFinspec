#!/bin/bash
export LLVM_DIR=/usr/bin/llvm-3.8/share/llvm/cmake
HOME=`pwd`
#mkdir -p instrumentor/build/
cd build/
#rm -rf *
rm monitor/libADFInstrumentationPass.so
cmake ..
make
#make VERBOSE=1
cd ..
rm *.o
# this needs modification 
BENCHS=$HOME/../benchmarks

#clang++ -Xclang -load -Xclang build/monitor/libADFInstrumentationPass.so -c example.cpp -o sparse_algebra_adf.o

clang++ -Xclang -load -Xclang build/monitor/libADFInstrumentationPass.so -c -Wall -O1 -Wno-unused-but-set-variable -I${BENCHS}/src -I${BENCHS}/include -I${BENCHS}/include/stm_stl  -DADF_STM -DADF -std=c++11 -pthread  -I${BENCHS}/include/atomic_ops -I${BENCHS}/tmmisc ${BENCHS}/dwarfs/sparse_algebra/sparse_algebra_adf.cpp
#exit
echo "COMPLETE 1st compile step"

g++ -c rtlib.cpp
echo "COMPLETE 2nd compile step"

g++ -o sparse_algebra_adf sparse_algebra_adf.o rtlib.o -L${BENCHS}/lib -litm -ladf -lpthread -lsfftw
echo "COMPLETE 3rd compile step"
#g++ -o  sparse_algebra_adf rtlib.o sparse_algebra_adf.o
echo "RUNNING"

./sparse_algebra_adf
