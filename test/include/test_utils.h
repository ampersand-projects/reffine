#ifndef TEST_INCLUDE_TEST_UTILS_H_
#define TEST_INCLUDE_TEST_UTILS_H_

#include <string>

#include <arrow/api.h>
#include <arrow/csv/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/c/bridge.h>

#include "reffine/pass/printer.h"
#include "reffine/pass/canonpass.h"
#include "reffine/pass/loopgen.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/engine/engine.h"

struct ArrowTable {
    ArrowSchema schema;
    ArrowArray array;

    ArrowTable(ArrowSchema schema, ArrowArray array)
        : schema(schema), array(array) {}
};

arrow::Result<ArrowTable> get_input_vector();
std::string print_output_vector(ArrowSchema, ArrowArray);

template<typename T>
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

template<typename T>
T compile_op(std::shared_ptr<reffine::Func> op)
{
    auto loop = reffine::LoopGen::Build(op);
    return compile_loop<T>(loop);
}

#endif  // TEST_INCLUDE_TEST_UTILS_H_
