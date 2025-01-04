#ifndef TEST_INCLUDE_TEST_UTILS_H_
#define TEST_INCLUDE_TEST_UTILS_H_

#include <arrow/api.h>
#include <arrow/c/bridge.h>
#include <arrow/csv/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <arrow/result.h>
#include <arrow/status.h>

#include <string>

#include "reffine/arrow/defs.h"
#include "reffine/engine/engine.h"
#include "reffine/pass/canonpass.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/pass/loopgen.h"
#include "reffine/pass/printer.h"
#include "reffine/pass/reffinepass.h"

arrow::Result<reffine::ArrowTable> get_input_vector();
std::string print_output_vector(ArrowSchema*, ArrowArray*);

template <typename T>
T compile_loop(std::shared_ptr<reffine::Func> loop)
{
    reffine::CanonPass::Build(loop);

    auto jit = reffine::ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("test", jit->GetCtx());
    reffine::LLVMGen::Build(loop, *llmod);
    if (llvm::verifyModule(*llmod)) {
        throw std::runtime_error("LLVM module verification failed!!!");
    }

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

#endif  // TEST_INCLUDE_TEST_UTILS_H_
