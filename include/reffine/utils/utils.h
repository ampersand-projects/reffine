#ifndef INCLUDE_REFFINE_UTILS_H_
#define INCLUDE_REFFINE_UTILS_H_

#include <string>

#include "reffine/arrow/table.h"
#include "reffine/base/log.h"
#include "reffine/engine/engine.h"
#include "reffine/pass/canonpass.h"
#include "reffine/pass/cemitter.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/pass/loopgen.h"
#include "reffine/pass/printer2.h"
#include "reffine/pass/readwritepass.h"
#include "reffine/pass/reffinepass.h"
#include "reffine/pass/scalarpass.h"

using namespace reffine;

template <typename T>
T compile_loop(shared_ptr<Func> loop, bool use_cemitter = true)
{
    LOG(INFO) << "Loop IR (raw):" << std::endl << loop->str() << std::endl;
    auto loop1 = CanonPass().eval(loop);
    LOG(INFO) << "Loop IR (canon):" << std::endl << loop1->str() << std::endl;
    auto loop2 = ReadWritePass().eval(loop1);
    LOG(INFO) << "Loop IR (readwrite):" << std::endl
              << loop2->str() << std::endl;
    auto loop3 = ScalarPass().eval(loop2);
    LOG(INFO) << "Loop IR (scalar):" << std::endl << loop3->str() << std::endl;

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("__" + loop->name, jit->GetCtx());
    if (use_cemitter) {
        auto ccode = CEmitter::Build(loop3);
        LOG(INFO) << "C Code:" << std::endl << ccode << std::endl;
        LLVMGen(*llmod).parse(ccode);
    } else {
        LLVMGen(*llmod).eval(loop3);
    }
    LOG(INFO) << "LLVM IR:" << std::endl
              << IRPrinter2::Build(*llmod) << std::endl;
    jit->AddModule(std::move(llmod));
    return jit->Lookup<T>(loop->name);
}

template <typename T>
T compile_op(std::shared_ptr<Func> op, bool vectorize = false)
{
    LOG(INFO) << "Reffine IR:" << std::endl << op->str() << std::endl;
    auto loopgen = LoopGen(nullptr, vectorize);
    loopgen.eval(op);
    return compile_loop<T>(loopgen.ctx().out_func);
}

#endif  // INCLUDE_REFFINE_UTILS_H_
