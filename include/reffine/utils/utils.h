#ifndef INCLUDE_REFFINE_UTILS_H_
#define INCLUDE_REFFINE_UTILS_H_

#include <string>

#include "reffine/arrow/defs.h"
#include "reffine/engine/engine.h"
#include "reffine/pass/canonpass.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/pass/loopgen.h"
#include "reffine/pass/printer.h"
#include "reffine/pass/reffinepass.h"
#include "reffine/pass/scalarpass.h"

using namespace reffine;

template <typename T>
T compile_loop(std::shared_ptr<Func> loop)
{
    CanonPass::Build(loop);
    auto exp_loop = LoadStoreExpand::Build(loop);
    auto elm_loop = NewGetElimination::Build(exp_loop);

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("test", jit->GetCtx());

    LLVMGen::Build(elm_loop, *llmod);
    jit->AddModule(std::move(llmod));
    return jit->Lookup<T>(loop->name);
}

template <typename T>
T compile_op(std::shared_ptr<Func> op)
{
    auto op_to_loop = OpToLoop::Build(op);
    auto loop = LoopGen::Build(op_to_loop);
    return compile_loop<T>(loop);
}

#endif  // INCLUDE_REFFINE_UTILS_H_
