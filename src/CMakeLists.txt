set(SRC_FILES
    ir/ir.cpp
    pass/printer.cpp
    pass/llvmgen.cpp
    engine/engine.cpp
)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs core executionengine support native orcjit object)

# Generate vinstr IR for JIT
#
# We have two commands that run scripts/gen_vinstr.sh because
# execute_process is for configure (cmake) and target is for build (make)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vinstr_str.cpp
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/gen_vinstr.sh ${CMAKE_CXX_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS pass/vinstr.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../include/reffine/pass/vinstr.h
)

add_library(reffine STATIC ${SRC_FILES} ${CMAKE_CURRENT_BINARY_DIR}/vinstr_str.cpp)

target_link_libraries(reffine
    ${llvm_libs}
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/arrow/cpp/build/debug/libarrow.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/arrow/cpp/build/debug/libarrow_bundled_dependencies.a
)
target_include_directories(reffine PUBLIC
    ${LLVM_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/arrow/cpp/src/
)
target_compile_options(reffine PRIVATE -Wall -Wextra -pedantic -Werror -Wno-unused-parameter)
