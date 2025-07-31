#ifndef INCLUDE_REFFINE_UTILS_H_
#define INCLUDE_REFFINE_UTILS_H_

#include <string>

#include "reffine/arrow/defs.h"
#include "reffine/base/log.h"
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
    loop = LoadStoreExpand::Build(loop);
    loop = NewGetElimination::Build(loop);

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("test", jit->GetCtx());
    LLVMGen::Build(loop, *llmod);
    jit->Optimize(*llmod);
    jit->AddModule(std::move(llmod));
    return jit->Lookup<T>(loop->name);
}

template <typename T>
T compile_op(std::shared_ptr<Func> op)
{
    auto loop = LoopGen::Build(op);
    return compile_loop<T>(loop);
}

#endif  // INCLUDE_REFFINE_UTILS_H_
