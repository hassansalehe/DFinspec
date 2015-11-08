#!/bin/bash
export LLVM_DIR=/usr/bin/llvm-3.8/share/llvm/cmake

HOME=`pwd`
BENCHS=$HOME/../benchmarks
SRC=$BENCHS/src
INC=$BENCHS/include
STMSTL=$BENCHS/include/stm_stl
ATOMICOPS=$BENCHS/include/atomic_ops
TMMISC=$BENCHS/tmmisc

mkdir -p build
cd build/

#rm -rf *
rm monitor/libADFInstrumentationPass.so

cmake ..
make
#make VERBOSE=1
cd ..
rm *.o
rm ${BENCHS}/obj/adf/adf.o

#clang++ -Xclang -load -Xclang build/monitor/libADFInstrumentationPass.so -c example.cpp -o sparse_algebra_adf.o


clang++ -Xclang -load -Xclang build/monitor/libADFTokenDetectorPass.so  -c -g -Wall -O3 -Wno-unused-but-set-variable -I${SRC} -I${INC} -I${STMSTL}  -DADF_STM -DADF -std=c++11 -pthread -I${ATOMICOPS} -I${TMMISC} ${BENCHS}/src/adf.cpp -o ${BENCHS}/obj/adf/adf.o

cd $BENCHS
make
cd $HOME

clang++ -Xclang -load -Xclang build/monitor/libADFInstrumentationPass.so -c -g -Wall -O3 -Wno-unused-but-set-variable -I${BENCHS}/src -I${BENCHS}/include -I${BENCHS}/include/stm_stl  -DADF_STM -DADF -std=c++11 -pthread  -I${BENCHS}/include/atomic_ops -I${BENCHS}/tmmisc ${BENCHS}/dwarfs/map_reduce/map_reduce_adf.cpp
#sparse_algebra/sparse_algebra_adf.cpp
#exit
echo "COMPLETE 1st compile step"

g++ -c -std=c++11 Logger.cpp
g++ -c -std=c++11 Callbacks.cpp
echo "COMPLETE 2nd compile step"

g++ -o sparse_algebra_adf map_reduce_adf.o Callbacks.o Logger.o -L${BENCHS}/lib -litm -ladf -lpthread -lsfftw
#g++ -o sparse_algebra_adf sparse_algebra_adf.o Callbacks.o Logger.o -L${BENCHS}/lib -litm -ladf -lpthread -lsfftw
echo "COMPLETE 3rd compile step"
#g++ -o  sparse_algebra_adf rtlib.o sparse_algebra_adf.o
echo "RUNNING"

./sparse_algebra_adf
