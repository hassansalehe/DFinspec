#!/bin/bash
export LLVM_DIR=/usr/bin/llvm-3.8/share/llvm/cmake

HOME=`pwd`

## Benchmarks-related directories ##
BENCHS_DIR=$HOME/benchmarks
SRC=$BENCHS_DIR/src
INC=$BENCHS_DIR/include
STMSTL=$BENCHS_DIR/include/stm_stl
ATOMICOPS=$BENCHS_DIR/include/atomic_ops
TMMISC=$BENCHS_DIR/tmmisc

#instrumentation directory
INSTR_DIR=$HOME/instrumentor
BIN_DIR=$HOME/bin

mkdir -p build
mkdir -p bin

cd build/

rm -rf *  > /dev/null 2>&1

## Build the LLVM passes for runtime and application instrumentations ##
cmake $INSTR_DIR
make
cp *.so $BIN_DIR/
g++ -c -std=c++11 $INSTR_DIR/Logger.cpp -o $BIN_DIR/Logger.o
g++ -c -std=c++11 $INSTR_DIR/Callbacks.cpp -o $BIN_DIR/Callbacks.o

## Build the ADF runtime and the dwarf benchmarks  ##
cd $BENCHS_DIR
make clean
rm ${BENCHS_DIR}/obj/adf/adf.o > /dev/null 2>&1

clang++ -Xclang -load -Xclang $BIN_DIR/libADFTokenDetectorPass.so  -c -g -Wall -O0 -Wno-unused-but-set-variable -I${SRC} -I${INC} -I${STMSTL}  -DADF_STM -DADF -std=c++11 -pthread -I${ATOMICOPS} -I${TMMISC} ${BENCHS_DIR}/src/adf.cpp -o ${BENCHS_DIR}/obj/adf/adf.o

make
cd $HOME
