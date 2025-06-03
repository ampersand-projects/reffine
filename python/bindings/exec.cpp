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
    m.def("compile_loop",
          [](std::shared_ptr<Func> fn) { return compile_loop(fn); });
    m.def("compile_op",
          [](std::shared_ptr<Func> fn) { return compile_op(fn); });
    m.def("print_llvm",
          [](std::shared_ptr<Func> fn) { return print_llvm(fn); });
}