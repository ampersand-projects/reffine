cmake_minimum_required(VERSION 3.13.4)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate vinstr IR for JIT
#
# We have two commands that run scripts/gen_vinstr.sh because
# execute_process is for configure (cmake) and target is for build (make)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vinstr_str.cpp
    COMMAND ${CMAKE_CXX_COMPILER} -emit-llvm -S ${CMAKE_CURRENT_SOURCE_DIR}/vinstr/internal.cpp -I ${CMAKE_CURRENT_SOURCE_DIR}/../include/ -o ${CMAKE_CURRENT_BINARY_DIR}/vinstr.ll
    COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND xxd -i vinstr.ll vinstr_str.cpp
    DEPENDS vinstr/internal.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../include/reffine/vinstr/vinstr.h
    COMMENT "Compile vinstr/internal.cpp to LLVM IR and embed into a char array in vinstr_str.cpp"
)

set(SRC_FILES
    arrow/base.cpp
    ir/ir.cpp
    iter/iter_space.cpp
    pass/printer.cpp
    pass/printer2.cpp
    pass/loopgen.cpp
    pass/reffinepass.cpp
    pass/canonpass.cpp
    pass/scalarpass.cpp
    pass/symanalysis.cpp
    pass/irclone.cpp
    pass/z3solver.cpp
    pass/llvmgen.cpp
    vinstr/internal.cpp
    vinstr/external.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/vinstr_str.cpp
    engine/engine.cpp
    engine/cuda_engine.cpp
    engine/memory.cpp
    builder/reffiner.cpp
)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package(Z3 REQUIRED CONFIG)
message(STATUS "Found Z3 ${Z3_VERSION_STRING}")
message(STATUS "Z3_DIR: ${Z3_DIR}")

add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs core executionengine support native orcjit object)

add_library(reffine SHARED ${SRC_FILES} ${CMAKE_CURRENT_BINARY_DIR}/vinstr_str.cpp)

target_link_libraries(reffine ${llvm_libs} ${Z3_LIBRARIES})
target_include_directories(reffine PUBLIC ${LLVM_INCLUDE_DIRS} ${Z3_CXX_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_options(reffine PRIVATE -Wall -Wextra -pedantic -Werror -Wno-unused-parameter ${Z3_COMPONENT_CXX_FLAGS})

if(ENABLE_CUDA)
    find_package(CUDA REQUIRED)
    llvm_map_components_to_libnames(llvm_nvptx_libs
        NVPTX
        NVPTXDesc
        NVPTXInfo
        NVPTXCodeGen
    )
    target_include_directories(reffine SYSTEM PUBLIC ${CUDA_INCLUDE_DIRS})
    target_link_libraries(reffine ${llvm_nvptx_libs} ${CUDA_CUDA_LIBRARY})
endif()
