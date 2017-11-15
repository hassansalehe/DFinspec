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

# Instrumentation passes directory
INSTR_DIR=$HOME/passes
BIN_DIR=$HOME/bin


### 1. ###
echo -e "\033[1;95mDFinspec: Building the ADFinspec tool.\033[m"
##  ##
cd $HOME
make  || { echo 'Building ADFinspec failed' ; exit 1; }

### 2. ###
mkdir -p .build
mkdir -p bin

cd .build/

rm -rf *  > /dev/null 2>&1

echo -e "\033[1;95mDFinspec: Building LLVM passes for runtime and application instrumentations.\033[m"
rm -rf $INSTR_DIR/Makefile $INSTR_DIR/*.so $INSTR_DIR/CMakeFile $INSTR_DIR/*.cmake > /dev/null 2>&1
cmake $INSTR_DIR
make
cp *.so $BIN_DIR/
g++ -c -O3 -std=c++11 $INSTR_DIR/Logger.cpp -I$INSTR_DIR/includes -I$INSTR_DIR/../common -o $BIN_DIR/Logger.o  || { echo 'Compiling Logger.cpp failed' ; exit 1; }
g++ -c -O3 -std=c++11 $INSTR_DIR/Callbacks.cpp -I$INSTR_DIR/includes -I$INSTR_DIR/../common -o $BIN_DIR/Callbacks.o  || { echo 'Compiling Callbacks.cpp failed' ; exit 1; }

### 3. ###
echo -e "\033[1;95mDFinspec: Building the ADF runtime and the dwarf benchmarks.\033[m"
cd $BENCHS_DIR
make clean
rm ${BENCHS_DIR}/obj/adf/adf.o > /dev/null 2>&1

clang++ -Xclang -load -Xclang $BIN_DIR/libADFTokenDetectorPass.so  -c -g -Wall -O0 -Wno-unused-but-set-variable -I${SRC} -I${INC} -I${STMSTL}  -DADF_STM -DADF -std=c++11 -pthread -I${ATOMICOPS} -I${TMMISC} ${BENCHS_DIR}/src/adf.cpp -o ${BENCHS_DIR}/obj/adf/adf.o  || { echo 'Compiling adf.cpp failed' ; exit 1; }

make  || { echo 'make failed' ; exit 1; }

if [ $? -eq 0 ]; then
    echo -e "\033[1;32mDFinspec: Done.\033[m"
else
    echo -e "\033[1;31mDFinspec: Fail.\033[m"
fi

cd $HOME

