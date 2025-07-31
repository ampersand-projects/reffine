cmake_minimum_required(VERSION 3.26)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(main)

option(ENABLE_CUDA "Enable CUDA support" OFF)
if(ENABLE_CUDA)
    find_package(CUDA REQUIRED)
    message(STATUS "CUDA support is enabled.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Arrow REQUIRED)

add_subdirectory(src)

add_executable(main main.cpp)
target_link_libraries(main reffine Arrow::arrow_shared)

add_subdirectory(third_party/pybind11)
add_subdirectory(python)

if(ENABLE_CUDA)
    target_compile_definitions(reffine PUBLIC ENABLE_CUDA)
    target_include_directories(reffine SYSTEM PUBLIC ${CUDA_INCLUDE_DIRS})
endif()
