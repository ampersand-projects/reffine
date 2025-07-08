#ifndef INCLUDE_REFFINE_UTILS_H_
#define INCLUDE_REFFINE_UTILS_H_

#include <string>

#include "reffine/engine/cuda_engine.h"
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
    LOG(INFO) << "Loop IR (raw):" << std::endl << loop->str() << std::endl;
    CanonPass::Build(loop);
    LOG(INFO) << "Loop IR (canon):" << std::endl << loop->str() << std::endl;
    loop = LoadStoreExpand::Build(loop);
    LOG(INFO) << "Loop IR (expand):" << std::endl << loop->str() << std::endl;
    loop = NewGetElimination::Build(loop);
    LOG(INFO) << "Loop IR (eliminate):" << std::endl
              << loop->str() << std::endl;

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("test", jit->GetCtx());
    LLVMGen::Build(loop, *llmod);
    LOG(INFO) << "LLVM IR (raw):" << std::endl
              << IRPrinter::Build(*llmod) << std::endl;
    jit->Optimize(*llmod);
    LOG(INFO) << "LLVM IR (optimized):" << std::endl
              << IRPrinter::Build(*llmod) << std::endl;
    jit->AddModule(std::move(llmod));
    return jit->Lookup<T>(loop->name);
}

template <typename T>
T compile_op(std::shared_ptr<Func> op)
{
    LOG(INFO) << "Reffine IR:" << std::endl << op->str() << std::endl;
    auto loop = LoopGen::Build(op);
    return compile_loop<T>(loop);
}

CUfunction compile_kernel(std::shared_ptr<Func>);

#endif  // INCLUDE_REFFINE_UTILS_H_
