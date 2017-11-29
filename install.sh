#/////////////////////////////////////////////////////////////////
#//  DFinspec: a lightweight non-determinism checking
#//          tool for shared memory DataFlow applications
#//
#//    Copyright (c) 2015 - 2017 Hassan Salehe Matar
#//      Copying or using this code by any means whatsoever
#//      without consent of the owner is strictly prohibited.
#//
#//   Contact: hmatar-at-ku-dot-edu-dot-tr
#//
#/////////////////////////////////////////////////////////////////
#!/bin/bash

HOME=`pwd`

# Instrumentation passes directory
INSTR_DIR=$HOME/src/passes
BIN_DIR=$HOME/bin
procNo=`cat /proc/cpuinfo | grep processor | wc -l`

### 1. Install OpenMP
echo -e "\033[1;95mDFinspec: Installing OpenMP runtime.\033[m"
git clone https://github.com/llvm-mirror/openmp.git openmp
cd openmp/runtime
mkdir -p build && cd build
cmake -G Ninja -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++	\
      -D CMAKE_BUILD_TYPE=Debug -D CMAKE_INSTALL_PREFIX:PATH=$BIN_DIR	\
      -D LIBOMP_OMPT_SUPPORT=on -D LIBOMP_OMPT_BLAME=on 		\
      -D LIBOMP_OMPT_TRACE=on ..
ninja -j$procNo
ninja install
### 2. Compile tool components
echo -e "\033[1;95mDFinspec: Building detection libraries, and LLVM passes for instrumentations.\033[m"

cd $HOME
mkdir -p .build
cd .build/
rm -rf *  > /dev/null 2>&1
cmake .. || { echo 'Building DFinspec failed' ; exit 1; }
make -j${procNo} || { echo 'Building DFinspec failed' ; exit 1; }


if [ $? -eq 0 ]; then
    echo -e "\033[1;32mDFinspec: Done.\033[m"
else
    echo -e "\033[1;31mDFinspec: Fail.\033[m"
fi

cd $HOME
echo -e "\033[1;32mDeleting temporary object files.\033[m"
find . -type f -name "*.o" | grep -v "obj/adf/" | xargs rm
