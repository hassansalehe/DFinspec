#/////////////////////////////////////////////////////////////////
#//  DFinspec: a lightweight non-determinism checking
#//          tool for shared memory DataFlow applications
#//
#//    Copyright (c) 2015 - 2018 Hassan Salehe Matar
#//      Copying or using this code by any means whatsoever
#//      without consent of the owner is strictly prohibited.
#//
#//   Contact: hmatar-at-ku-dot-edu-dot-tr
#//
#/////////////////////////////////////////////////////////////////
#!/bin/bash

HOME=`pwd`

## Benchmarks-related directories ##
BENCHS_DIR=$HOME/src/tests/adf_benchmarks
SRC=$BENCHS_DIR/src
INC=$BENCHS_DIR/include
STMSTL=$BENCHS_DIR/include/stm_stl
ATOMICOPS=$BENCHS_DIR/include/atomic_ops
TMMISC=$BENCHS_DIR/tmmisc

# Instrumentation passes directory
INSTR_DIR=$HOME/src/passes
BIN_DIR=$HOME/bin
procNo=`cat /proc/cpuinfo | grep processor | wc -l`

### 1. Compile tool components
echo -e "\033[1;95mDFinspec: Building detection libraries, and LLVM passes for instrumentations.\033[m"

cd $HOME
mkdir -p .build
cd .build/
rm -rf *  > /dev/null 2>&1
cmake .. || { echo 'Building DFinspec failed' ; exit 1; }
make -j${procNo} || { echo 'Building DFinspec failed' ; exit 1; }

### 2. Buiding ADF runtime
echo -e "\033[1;95mDFinspec: Building the ADF runtime and the dwarf benchmarks.\033[m"
cd $BENCHS_DIR
make clean
rm ${BENCHS_DIR}/obj/adf/adf.o > /dev/null 2>&1

clang++ -Xclang -load -Xclang $BIN_DIR/libADFTokenDetectorPass.so  -c -g -Wall -O0 -Wno-unused-but-set-variable -I${SRC} -I${INC} -I${STMSTL}  -DADF_STM -DADF -std=c++11 -pthread -I${ATOMICOPS} -I${TMMISC} ${BENCHS_DIR}/src/adf.cpp -o ${BENCHS_DIR}/obj/adf/adf.o  || { echo 'Compiling adf.cpp failed' ; exit 1; }

make || { echo 'make failed' ; exit 1; }

if [ $? -eq 0 ]; then
    echo -e "\033[1;32mDFinspec: Done.\033[m"
else
    echo -e "\033[1;31mDFinspec: Fail.\033[m"
fi

cd $HOME
echo -e "\033[1;32mDeleting temporary object files.\033[m"
find . -type f -name "*.o" | grep -v "obj/adf/" | xargs rm
