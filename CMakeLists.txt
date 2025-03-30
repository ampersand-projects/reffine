cmake_minimum_required(VERSION 3.26)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(main)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Arrow REQUIRED)

add_subdirectory(src)

add_executable(main main.cpp)
target_link_libraries(main reffine Arrow::arrow_shared)

add_subdirectory(third_party/googletest)
enable_testing()
add_subdirectory(test)

option(ENABLE_CUDA "Enable CUDA support" ON)
if(ENABLE_CUDA)
    find_package(CUDA REQUIRED)
    target_include_directories(reffine SYSTEM PUBLIC ${CUDA_INCLUDE_DIRS})
endif()