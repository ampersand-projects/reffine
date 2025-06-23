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

template <typename T>
T compile_loop(std::shared_ptr<reffine::Func> loop)
{
    reffine::CanonPass::Build(loop);
    auto exp_loop = reffine::LoadStoreExpand::Build(loop);
    auto elm_loop = reffine::NewGetElimination::Build(exp_loop);

    auto jit = reffine::ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("test", jit->GetCtx());

    reffine::LLVMGen::Build(elm_loop, *llmod);
    jit->AddModule(std::move(llmod));
    return jit->Lookup<T>(loop->name);
}

template <typename T>
T compile_op(std::shared_ptr<reffine::Func> op)
{
    auto op_to_loop = reffine::OpToLoop::Build(op);
    auto loop = reffine::LoopGen::Build(op_to_loop);
    return compile_loop<T>(loop);
}

#endif  // INCLUDE_REFFINE_UTILS_H_
