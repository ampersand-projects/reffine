#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "reffine/builder/reffiner.h"
#include "reffine/engine/engine.h"
#include "reffine/pass/canonpass.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/pass/loopgen.h"
#include "reffine/pass/printer.h"
#include "reffine/pass/reffinepass.h"
#include "reffine/pass/scalarpass.h"
#include "reffine/pass/symanalysis.h"
#include "reffine/pass/z3solver.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

namespace py = pybind11;

string to_string(shared_ptr<Func> fn) { return IRPrinter::Build(fn); }
string to_string(llvm::Module& m) { return IRPrinter::Build(m); }

template<typename Output, typename... Inputs>
Output execute_loop(std::shared_ptr<Func> fn, Output output, Inputs... inputs) {
    CanonPass::Build(fn);
    auto exp_loop = LoadStoreExpand::Build(fn);
    auto ngelm_loop = NewGetElimination::Build(exp_loop);

    auto jit = ExecEngine::Get();
    auto llmod = std::make_unique<llvm::Module>("test", jit->GetCtx());
    LLVMGen::Build(ngelm_loop, *llmod);

    jit->AddModule(std::move(llmod));

    using FuncType = void (*)(Output, Inputs...);
    auto query = jit->Lookup<FuncType>(fn->name);
    query(output, (inputs)...);

    return output;
}

template<typename... Args>
void execute_op(shared_ptr<Func> fn, Args... inputs)
{
    auto fn2 = OpToLoop::Build(fn);
    auto loop = LoopGen::Build(fn2);

    // execute_loop<Args...>(loop, inputs...);
}

std::unique_ptr<llvm::Module> compile_loop(shared_ptr<Func> fn)
{
    CanonPass::Build(fn);
    auto exp_loop = LoadStoreExpand::Build(fn);
    auto ngelm_loop = NewGetElimination::Build(exp_loop);

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("test", jit->GetCtx());
    LLVMGen::Build(ngelm_loop, *llmod);

    return llmod;
}

std::unique_ptr<llvm::Module> compile_op(shared_ptr<Func> fn)
{
    auto fn2 = OpToLoop::Build(fn);
    auto loop = LoopGen::Build(fn2);

    return compile_loop(loop);
}

string print_llvm(shared_ptr<Func> fn)
{
    auto m = compile_op(fn);
    return IRPrinter::Build(*m);
}

PYBIND11_MODULE(exec, m)
{
    m.def("to_string", [](std::shared_ptr<Func> fn) { return to_string(fn); });

    m.def("execute_loop", [](std::shared_ptr<Func> fn, py::args args) { 
        // if (args.size() == 2) {

        //     auto output_buf = args[0].cast<py::list>();
        //     auto input_buf = args[1].cast<py::list>();

        //     std::vector<int64_t> output_buf(output_list.size());
        //     std::vector<int64_t> input_buf(input_list.size());

        //     // Copy from Python list to C++ vector
        //     for (size_t i = 0; i < input_list.size(); ++i)
        //         input_buf[i] = py::cast<int64_t>(input_list[i]);

        //     int64_t* out_ptr = output_buf.data();
        //     int64_t* in_ptr  = input_buf.data();

        //     execute_loop(fn, out_ptr, in_ptr);

        //     // Copy back from C++ buffer to Python list
        //     for (size_t i = 0; i < output_list.size(); ++i)
        //         output_list[i] = py::int_(output_buf[i]);

        //     return output_list;
        // }

        // throw std::runtime_error("Unsupported argument types or count. Only supports 1 int64 input and int64 output atm");
    });

    m.def("execute_op",
          [](std::shared_ptr<Func> fn) { return execute_op(fn); });
    m.def("compile_loop",
          [](std::shared_ptr<Func> fn) { return compile_loop(fn); });
    m.def("compile_op",
          [](std::shared_ptr<Func> fn) { return compile_op(fn); });
    m.def("print_llvm",
          [](std::shared_ptr<Func> fn) { return print_llvm(fn); });
}
