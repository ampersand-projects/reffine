pybind11_add_module(ir bindings/ir.cpp)

target_include_directories(ir PUBLIC reffine)
target_link_libraries(ir PRIVATE reffine)
