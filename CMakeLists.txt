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

cmake_minimum_required(VERSION 3.4.3)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

## Set destination directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

## Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/passes/includes)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/detector)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/bin/include)
link_directories(${CMAKE_SOURCE_DIR}/bin)

# List of source files for the checker module
set(DETECTOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/detector)
add_executable(DFchecker ${DETECTOR_DIR}/main.cpp
    ${DETECTOR_DIR}/checker.cpp ${DETECTOR_DIR}/validator.cpp)

# List source files for libraries and compiler passes.
add_library(ADFInstrumentPass MODULE src/passes/AdfInstrumentor.cpp)
add_library(ADFTokenDetectorPass MODULE src/passes/TokenDetector.cpp)
add_library(OpenMPInstrumentPass MODULE src/passes/OpenMPInstrumentor.cpp )
add_library(Logger STATIC src/passes/Logger.cpp)
add_library(Callbacks STATIC src/passes/Callbacks.cpp)
add_library(OpenmpCallbacks STATIC
     src/passes/OpenmpCallbacks.cpp
     src/passes/Logger.cpp)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(ADFInstrumentPass PRIVATE cxx_range_for cxx_auto_type)
target_compile_features(ADFTokenDetectorPass PRIVATE cxx_range_for cxx_auto_type)
target_compile_features(OpenMPInstrumentPass PRIVATE cxx_range_for cxx_auto_type)
target_compile_features(DFchecker PRIVATE cxx_range_for cxx_auto_type)
target_compile_features(Logger PRIVATE cxx_range_for cxx_auto_type)

# Add compiler flags. LLVM is (typically) built with no C++ RTTI.
set_target_properties(
     ADFInstrumentPass ADFTokenDetectorPass OpenMPInstrumentPass
     DFchecker Logger OpenmpCallbacks Callbacks PROPERTIES
     COMPILE_FLAGS "-g -O3 -std=c++11 -fno-rtti -fPIC")

set_target_properties(OpenmpCallbacks PROPERTIES
     COMPILE_FLAGS " -DLIBOMP_OMPT_SUPPORT=on -fPIC -g -O3 -std=c++11 -fno-rtti -fopenmp")

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(
    DFchecker
    ADFInstrumentPass
    ADFTokenDetectorPass
    OpenMPInstrumentPass PROPERTIES
    LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
