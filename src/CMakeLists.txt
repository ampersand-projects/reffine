cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(REFFINE_HEADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../include/")
add_compile_definitions(REFFINE_SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")

# Generate vinstr IR for JIT
#
# We have two commands that run scripts/gen_vinstr.sh because
# execute_process is for configure (cmake) and target is for build (make)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vinstr_str.cpp
    COMMAND ${CMAKE_CXX_COMPILER} -emit-llvm -S
            ${CMAKE_CURRENT_SOURCE_DIR}/vinstr/internal.cpp
            -I ${CMAKE_CURRENT_SOURCE_DIR}/../include/
            -o ${CMAKE_CURRENT_BINARY_DIR}/vinstr.ll
    COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND xxd -i vinstr.ll vinstr_str.cpp
    DEPENDS vinstr/internal.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/reffine/vinstr/vinstr.h
    COMMENT "Compile vinstr/internal.cpp to LLVM IR and embed into a char array in vinstr_str.cpp"
)

set(SRC_FILES
    base/type.cpp
    arrow/base.cpp
    ir/ir.cpp
    iter/iter_space.cpp
    pass/printer.cpp
    pass/printer2.cpp
    pass/cemitter.cpp
    pass/loopgen.cpp
    pass/reffinepass.cpp
    pass/canonpass.cpp
    pass/scalarpass.cpp
    pass/symanalysis.cpp
    pass/irclone.cpp
    pass/z3solver.cpp
    pass/llvmgen.cpp
    pass/mlirgen.cpp
    vinstr/internal.cpp
    vinstr/external.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/vinstr_str.cpp
    engine/engine.cpp
    engine/cuda_engine.cpp
    engine/memory.cpp
    builder/reffiner.cpp
)

find_package(LLVM 21.1.0 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package(MLIR REQUIRED CONFIG)
message(STATUS "Found MLIR ${MLIR_PACKAGE_VERSION}")
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

find_package(Z3 REQUIRED CONFIG)
message(STATUS "Found Z3 ${Z3_VERSION_STRING}")
message(STATUS "Z3_DIR: ${Z3_DIR}")

add_definitions(${LLVM_DEFINITIONS})

add_library(reffine SHARED ${SRC_FILES} ${CMAKE_CURRENT_BINARY_DIR}/vinstr_str.cpp)

target_include_directories(reffine PUBLIC
    ${LLVM_INCLUDE_DIRS}
    ${MLIR_INCLUDE_DIRS}
    ${Z3_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# link_directories(${LLVM_LIBRARY_DIR})


set(MLIR_LIBS
    MLIRIR              # <-- Add this (core MLIR library)
    MLIRSupport         # <-- Add this
    MLIRAnalysis
    MLIRArithDialect
    MLIRBuiltinToLLVMIRTranslation
    MLIRControlFlowDialect
    MLIRFuncDialect
    MLIRLLVMDialect
    MLIRLLVMToLLVMIRTranslation
    MLIRMathDialect
    MLIRMemRefDialect
    MLIRParser
    MLIRPass
    MLIRSCFDialect
    MLIRTargetLLVMIRExport
    MLIRTransforms
)

link_directories(${LLVM_LIBRARY_DIR})

target_link_libraries(reffine PUBLIC
    LLVM
    ${MLIR_LIBS}
    ${Z3_LIBRARIES}
)

target_compile_options(reffine PRIVATE
    -Wall -Wextra -pedantic -Werror -Wno-unused-parameter ${Z3_COMPONENT_CXX_FLAGS}
)

if(ENABLE_CUDA)
    find_package(CUDA REQUIRED)
    target_include_directories(reffine SYSTEM PUBLIC ${CUDA_INCLUDE_DIRS})

    target_link_libraries(reffine PUBLIC
        # LLVMNVPTXCodeGen
        # LLVMNVPTXDesc
        # LLVMNVPTXInfo
        ${CUDA_CUDA_LIBRARY}
    )
endif()
