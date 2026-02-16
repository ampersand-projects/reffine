#include <pybind11/pybind11.h>
namespace py = pybind11;

void init_ir(py::module_ &);
void init_exec(py::module_ &);

PYBIND11_MODULE(pyreffine, m)
{
    auto ir = m.def_submodule("ir", "IR bindings");
    init_ir(ir);

    auto exec = m.def_submodule("exec", "Execution engine bindings");
    init_exec(exec);
}
