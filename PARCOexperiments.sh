#!/bin/bash
#################################################
#                                               #
#   For running experiments for the PARCO       #
#         research publication                  #
#                                               #
#     (c) 2017 - Hassan Salehe Matar            #
#                                               #
#################################################

export LLVM_DIR=/usr/bin/llvm-3.8/share/llvm/cmake

HOME=`pwd`

## Benchmarks-related directories ##
BENCHS_DIR=$HOME/benchmarks
SRC=$BENCHS_DIR/src
INC=$BENCHS_DIR/include
STMSTL=$BENCHS_DIR/include/stm_stl
ATOMICOPS=$BENCHS_DIR/include/atomic_ops
TMMISC=$BENCHS_DIR/tmmisc

OBJ=$BENCHS_DIR/obj
LIB=$BENCHS_DIR/lib

AOPTIONS="-g -c -Wall -O0 -DADF_STM -DADF -std=c++11 -pthread -I$INC -I$STMSTL"
GetTime="/usr/bin/time -f ExecTime%e "

# Instrumentation directory
INSTR_DIR=$HOME/instrumentor
BIN_DIR=$HOME/bin

buildLibraries(){
### 1. ###
echo -e "\033[1;95mDFinspec: Building the ADFinspec tool.\033[m"
##  ##
cd $HOME
make  || { echo 'Building ADFinspec failed' ; exit 1; }

### 2. ###
mkdir -p build
mkdir -p bin

cd build/

rm -rf *  > /dev/null 2>&1

echo -e "\033[1;95mDFinspec: Building the LLVM passes for runtime and application instrumentations.\033[m"
rm -rf $INSTR_DIR/Makefile $INSTR_DIR/*.so $INSTR_DIR/CMakeFile $INSTR_DIR/*.cmake > /dev/null 2>&1
cmake $INSTR_DIR
make
cp *.so $BIN_DIR/
g++ -c -std=c++11 $INSTR_DIR/Logger.cpp -I$INSTR_DIR/../DFchecker -o $BIN_DIR/Logger.o  || { echo 'Compiling Logger.cpp failed' ; exit 1; }
g++ -c -std=c++11 $INSTR_DIR/Callbacks.cpp -I$INSTR_DIR/../DFchecker -o $BIN_DIR/Callbacks.o  || { echo 'Compiling Callbacks.cpp failed' ; exit 1; }

### 3. ###
echo -e "\033[1;95mDFinspec: Building the ADF runtime and the dwarf benchmarks.\033[m"
cd $BENCHS_DIR
make clean
rm ${BENCHS_DIR}/obj/adf/adf.o > /dev/null 2>&1
}

buildRuntime() {
clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/taskqueue.cpp -o ${OBJ}/adf/taskqueue.o
clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/threadpool.cpp -o ${OBJ}/adf/threadpool.o

clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/worksteal.cpp -o ${OBJ}/adf/worksteal.o

clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/statistics.cpp -o ${OBJ}/adf/statistics.o

clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/testscheduler.cpp -o ${OBJ}/adf/testscheduler.o

clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/common.cpp -o ${OBJ}/adf/common.o

clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/buffer.cpp -o ${OBJ}/adf/buffer.o

clang++ ${AOPTIONS} -I${SRC} -I${ATOMICOPS} -I${TMMISC} ${BENCHS_DIR}/src/adf.cpp -o ${BENCHS_DIR}/obj/adf/adf.o  || { echo 'Compiling adf.cpp failed' ; exit 1; }

  ar rcs ${LIB}/libadf.a ${OBJ}/adf/taskqueue.o ${OBJ}/adf/threadpool.o ${OBJ}/adf/adf.o ${OBJ}/adf/worksteal.o ${OBJ}/adf/statistics.o ${OBJ}/adf/testscheduler.o ${OBJ}/adf/common.o ${OBJ}/adf/buffer.o

}

instrRuntime() {
clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/taskqueue.cpp -o ${OBJ}/adf/taskqueue.o
clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/threadpool.cpp -o ${OBJ}/adf/threadpool.o

clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/worksteal.cpp -o ${OBJ}/adf/worksteal.o

clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/statistics.cpp -o ${OBJ}/adf/statistics.o

clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/testscheduler.cpp -o ${OBJ}/adf/testscheduler.o

clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/common.cpp -o ${OBJ}/adf/common.o

clang++ ${AOPTIONS} -I${SRC} -I${INC}/atomic_ops -I${TMMISC} ${SRC}/buffer.cpp -o ${OBJ}/adf/buffer.o

clang++ -Xclang -load -Xclang $BIN_DIR/libADFTokenDetectorPass.so  ${AOPTIONS} -I${SRC} -I${INC} -I${STMSTL} -I${ATOMICOPS} -I${TMMISC} ${BENCHS_DIR}/src/adf.cpp -o ${BENCHS_DIR}/obj/adf/adf.o  || { echo 'Compiling adf.cpp failed' ; exit 1; }

ar rcs ${LIB}/libadf.a ${OBJ}/adf/taskqueue.o ${OBJ}/adf/threadpool.o ${OBJ}/adf/adf.o ${OBJ}/adf/worksteal.o ${OBJ}/adf/statistics.o ${OBJ}/adf/testscheduler.o ${OBJ}/adf/common.o ${OBJ}/adf/buffer.o

}
### 4. Conduct experiments for branch_bound ###

runOneBenchmark() {

  BranchName=$1
  BranchHome=$BENCHS_DIR/dwarfs/${BranchName}

  echo "======Running: ${BranchName}  ====="
  # compile without instrumentation
  buildRuntime
  baseTime=0;
  for i in {1..10}; do
    temp=$(${GetTime} clang++ ${AOPTIONS} -I${BENCHS_DIR}/src -I$INC/atomic_ops \
      -I$TMMISC $BranchHome/${BranchName}_adf.cpp -o $BranchHome/${BranchName}_adf.o 2>&1 |  \
      grep ExecTime | sed 's/ExecTime//g')
    baseTime=`calc $baseTime+$temp`
    echo "Base step $i"
  done
  baseTime=`calc $baseTime/10`

  clang++ -o $BranchHome/${BranchName}_adf_base $BranchHome/${BranchName}_adf.o     \
           -L$BENCHS_DIR/lib -litm -ladf -lpthread -lsfftw

  ### compile with instrumentation ###
  instrRuntime
  instrTime=0;
  for i in {1..10}; do
    temp=$(${GetTime} clang++ -Xclang -load -Xclang ${BENCHS_DIR}/../bin/libADFInstrumentationPass.so  \
      ${AOPTIONS} -I${BENCHS_DIR}/src -I$INC/atomic_ops -I$TMMISC $BranchHome/${BranchName}_adf.cpp -o   \
      $BranchHome/${BranchName}_adf.o 2>&1 | grep ExecTime | sed 's/ExecTime//g')

    instrTime=`calc $instrTime+$temp`
    echo "Instr step $i"
  done
  instrTime=`calc $instrTime/10`

  clang++ -o $BranchHome/${BranchName}_adf_instr $BranchHome/${BranchName}_adf.o  \
     $BENCHS_DIR/../bin/Callbacks.o $BENCHS_DIR/../bin/Logger.o -L$BENCHS_DIR/lib -litm -ladf -lpthread -lsfftw

  # calculate instrumentation slowdown
  instrSlowdown=`calc ${instrTime}/${baseTime} | printf "%0.2f\n"`

  # set input size

  ## run the uninstrumented version
  cd ${BranchHome}
  ./${BranchName}_adf_base ${inputArgs} > temp
  echo "Base run is over"

  normalExecTime=`cat temp | grep "ADF running time" | sed 's/ADF run.*= //g' | sed 's/ ms$//g'`
  echo "Exec time"
  normalExecTime=`calc $normalExecTime/1000.0`

  # run the instrumented version
  cd ${BranchHome}
  rm -rf Tracelog* HBlog*
  echo "./${BranchName}_adf_instr ${inputArgs} > temp"
  ./${BranchName}_adf_instr ${inputArgs} > temp

  echo "Insrumented has run"
  numTasks=`cat temp | grep "Tasks executed" | sed 's/[^0-9]*//g'`
  logTime=`cat temp | grep "ADF running time" | sed 's/ADF run.*= //g' | sed 's/ ms$//g'`
  logTime=`calc $logTime/1000.0`

  # calculate the instrumentation overhead
  instrOverhead=`calc $logTime/$normalExecTime | printf "%0.1f\n"`

  # Get the log file size
  logSize=`du -h Tracelog* | awk '{print $1}'`

  echo "Now doing nondeterminism detection"

  # Do race detection analysis
  ../../../ADFinspec Tracelog* HBlog* ${BranchName}_adf.cpp.iir > checkerResult.txt
  detectionTime=`cat checkerResult.txt | grep "Checker execution time" | sed 's/Checker exec.* time: //g' | sed 's/ millis.*$//g'`
  detectionTime=`calc $detectionTime/1000`

  # print the whole result
  echo -e "${BranchName}: ${baseTime}, ${instrTime}, ${instrSlowdown}, ${inputSize}, ${numTasks}, ${normalExecTime}, ${logTime}, ${instrOverhead}, ${logSize}, ${detectionTime}"
}

buildRuntime
instrRuntime
make > /dev/null 2>&1 || { echo 'make failed' ; exit 1; }

# 1
inputSize="500 nodes"
inputArgs="-f smallcontent.txt"
runOneBenchmark branch_bound

inputSize="2000 nodes"
inputArgs="-f largecontent.txt"
runOneBenchmark branch_bound

# 2
inputSize="131072 bits"
inputArgs="-f smallcontent.txt"
runOneBenchmark combinatorial_logic

inputSize="2097152 bits"
inputArgs="-f largcontent.txt"
runOneBenchmark combinatorial_logic

# 3
nputSize="1000x1000 matrix"
inputArgs="-d 10 -b 100 -n 1"
runOneBenchmark dense_algebra

nputSize="2000x2000 matrix"
inputArgs="-d 20 -b 100 -n 1"
runOneBenchmark dense_algebra

# 4
nputSize="104M characters"
inputArgs="-f smallcontent.txt -n 1"
runOneBenchmark finite_state_machine

nputSize="800M characters"
inputArgs="-f largecontent.txt -n 1"
runOneBenchmark finite_state_machine

# 5
nputSize="2000 states"
inputArgs="-f smallcontent.txt -n 1"
runOneBenchmark graph_models

nputSize="4000 states"
inputArgs="-f largecontent.txt -n 1"
runOneBenchmark graph_models

# 6
nputSize="146729 words"
inputArgs="-f smallcontent.txt -n 1"
runOneBenchmark map_reduce

nputSize="1173848 words"
inputArgs="-f largecontent.txt -n 1"
runOneBenchmark map_reduce

# 7
nputSize="1250x1250 matrix"
inputArgs="-b 50 -s 25"
runOneBenchmark sparse_algebra

nputSize="2000x2000 matrix"
inputArgs="-b 50 -s 40"
runOneBenchmark sparse_algebra

# 8
nputSize="128x128 matrix"
inputArgs="-e 128"
runOneBenchmark spectral_methods

nputSize="256x256 matrix"
inputArgs="-e 256"
runOneBenchmark spectral_methods

# 9
nputSize="64x64 matrix"
inputArgs="-s 64"
runOneBenchmark structured_grid

nputSize="128x128 matrix"
inputArgs="-s 128"
runOneBenchmark structured_grid

# 10
nputSize="21333 vertices"
inputArgs="-f mediumcontent.txt"
runOneBenchmark unstructured_grid

nputSize="213333 vertices"
inputArgs="-f largecontent.txt"
runOneBenchmark unstructured_grid

if [ $? -eq 0 ]; then
    echo -e "\033[1;32mDFinspec: Done.\033[m"
else
    echo -e "\033[1;31mDFinspec: Fail.\033[m"
fi

cd $HOME

