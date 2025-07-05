#include "reffine/utils/utils.h"

CUfunction compile_kernel(std::shared_ptr<Func> fn)
{
    reffine::CanonPass::Build(fn);

    auto jit = reffine::ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("foo", jit->GetCtx());
    reffine::LLVMGen::Build(fn, *llmod);
    jit->Optimize(*llmod);

    auto cuda_engine = CudaEngine::Get();
    auto cuda_module = cuda_engine->Build(*llmod);
    return cuda_engine->Lookup(cuda_module, llmod->getName().str());
}
