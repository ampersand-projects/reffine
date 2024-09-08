cmake_minimum_required(VERSION 3.13.4)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(reffine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS} include)
add_definitions(${LLVM_DEFINITIONS})

set(SRC_FILES
    src/ir/ir.cpp
    src/pass/printer.cpp
    src/pass/llvmgen.cpp
    main.cpp
)

# Now build our tools
add_executable(reffine ${SRC_FILES})

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs core executionengine support native orcjit object)

# Link against LLVM libraries
target_link_libraries(reffine ${llvm_libs})

target_compile_options(reffine PRIVATE -Wall -Wextra -pedantic -Werror -Wno-unused-parameter)
