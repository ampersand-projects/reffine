#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "reffine/utils/utils.h"

using namespace std;
using namespace reffine;

namespace py = pybind11;

string to_string(shared_ptr<Func> fn) { return IRPrinter2::Build(fn); }

std::vector<void*> run(void* query, std::vector<void*> inputs)
{
    switch (inputs.size()) {
        case 1: {
            using FuncType = void (*)(void*, void*);
            auto query2 = reinterpret_cast<FuncType>(query);
            query2(inputs[0], inputs[0]);
            break;
        }
        case 2: {
            using FuncType = void (*)(void*, void*, void*);
            auto query2 = reinterpret_cast<FuncType>(query);
            query2(inputs[0], inputs[0], inputs[1]);
            break;
        }
        case 3: {
            using FuncType = void (*)(void*, void*, void*, void*);
            auto query2 = reinterpret_cast<FuncType>(query);
            query2(inputs[0], inputs[0], inputs[1], inputs[2]);
            break;
        }
        default:
            throw std::runtime_error("Unsupported number of inputs.");
    }
    return inputs;
}

void init_exec(py::module_& m)
{
    m.def("to_string", [](std::shared_ptr<Func> fn) { return to_string(fn); });

    m.def("run", [](void* fn, std::vector<py::array> inputs) {
        std::vector<void*> input_ptrs;
        for (auto& input : inputs) {
            auto input_buf = input.request();
            input_ptrs.push_back(input_buf.ptr);
        }
        run(fn, input_ptrs);

        return inputs;
    });

    m.def("run", [](void* fn, std::vector<py::capsule> inputs) {
        std::vector<void*> input_ptrs;
        for (auto& in : inputs) { input_ptrs.push_back(in.get_pointer()); }
        run(fn, input_ptrs);

        return inputs;
    });

    m.def("compile_loop",
          [](std::shared_ptr<Func> fn) { return compile_loop<void*>(fn); });

    m.def("compile_op",
          [](std::shared_ptr<Func> fn) { return compile_op<void*>(fn); });
}
