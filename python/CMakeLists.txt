pybind11_add_module(pyreffine
    bindings/pyreffine.cpp
    bindings/ir.cpp
    bindings/exec.cpp
)

target_include_directories(pyreffine PUBLIC reffine)
target_link_libraries(pyreffine PRIVATE reffine)
