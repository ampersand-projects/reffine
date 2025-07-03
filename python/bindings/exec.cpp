#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "reffine/utils/utils.h"

using namespace std;
using namespace reffine;

namespace py = pybind11;

string to_string(shared_ptr<Func> fn) { return IRPrinter::Build(fn); }

template <typename Output, typename... Inputs>
Output execute_query(void* query, Output output, Inputs... inputs)
{
    CanonPass::Build(fn);
    auto exp_loop = LoadStoreExpand::Build(fn);
    auto ngelm_loop = NewGetElimination::Build(exp_loop);

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("test", jit->GetCtx());
    LLVMGen::Build(ngelm_loop, *llmod);

    return llmod;
}

PYBIND11_MODULE(exec, m)
{
    m.def("to_string", [](std::shared_ptr<Func> fn) { return to_string(fn); });

    m.def("execute_query",
          [](void* fn, py::array output, std::vector<py::array> inputs) {
              auto output_buf = output.request();
              auto output_ptr = output_buf.ptr;

              std::vector<void*> input_ptrs;
              for (auto& input : inputs) {
                  auto input_buf = input.request();
                  input_ptrs.push_back(input_buf.ptr);
              }

              if (input_ptrs.size() == 1) {
                  execute_query(fn, output_ptr, output_ptr, input_ptrs[0]);
              } else if (input_ptrs.size() == 2) {
                  execute_query(fn, output_ptr, output_ptr, input_ptrs[0],
                                input_ptrs[1]);
              } else if (input_ptrs.size() == 3) {
                  execute_query(fn, output_ptr, output_ptr, input_ptrs[0],
                                input_ptrs[1], input_ptrs[2]);
              } else {
                  throw std::runtime_error("Unsupported number of inputs.");
              }

              return output;
          });

    m.def("execute_query", [](void* fn, py::capsule output,
                              std::vector<py::capsule> inputs) {
        auto out_arr = static_cast<ArrowArray*>(output.get_pointer());

        std::vector<ArrowArray*> input_ptrs;
        for (auto& in : inputs) {
            auto in_arr = static_cast<ArrowArray*>(in.get_pointer());
            input_ptrs.push_back(in_arr);
        }

        if (input_ptrs.size() == 1) {
            execute_query(fn, out_arr, out_arr, input_ptrs[0]);
        } else if (input_ptrs.size() == 2) {
            execute_query(fn, out_arr, out_arr, input_ptrs[0], input_ptrs[1]);
        } else if (input_ptrs.size() == 3) {
            execute_query(fn, out_arr, out_arr, input_ptrs[0], input_ptrs[1],
                          input_ptrs[2]);
        } else {
            throw std::runtime_error("Unsupported number of inputs.");
        }

        return output;
    });

    m.def("compile_loop",
          [](std::shared_ptr<Func> fn) { return compile_loop<void*>(fn); });

    m.def("compile_op",
          [](std::shared_ptr<Func> fn) { return compile_op<void*>(fn); });
}
