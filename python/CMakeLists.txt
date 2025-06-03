pybind11_add_module(ir bindings/ir.cpp)
pybind11_add_module(exec bindings/exec.cpp)

target_include_directories(ir PUBLIC reffine)
target_include_directories(exec PUBLIC reffine)

target_link_libraries(exec PRIVATE reffine)
target_link_libraries(ir PRIVATE reffine)
