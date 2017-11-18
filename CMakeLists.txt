cmake_minimum_required(VERSION 3.1)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/includes)

add_library(ADFInstrumentationPass MODULE
    # List your source files here.
    AdfInstrumentor.cpp
)

add_library(ADFTokenDetectorPass MODULE
    # List your source files here.
    TokenDetector.cpp
)

add_library(OpenMPInstrumentorPass MODULE
    # List your source files here.
    OpenMPInstrumentor.cpp
)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(ADFInstrumentationPass PRIVATE cxx_range_for cxx_auto_type)
target_compile_features(ADFTokenDetectorPass PRIVATE cxx_range_for cxx_auto_type)
target_compile_features(OpenMPInstrumentorPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that.
set_target_properties(ADFInstrumentationPass PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)

set_target_properties(ADFTokenDetectorPass PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)

set_target_properties(OpenMPInstrumentorPass PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(ADFInstrumentationPass PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )

    set_target_properties(ADFTokenDetectorPass PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )

endif(APPLE)
