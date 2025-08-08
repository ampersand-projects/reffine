#include "reffine/utils/utils.h"

namespace reffine {

CUfunction compile_kernel(std::shared_ptr<Func> fn)
{
    CanonPass::Build(fn);

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("foo", jit->GetCtx());
    LLVMGen::Build(fn, *llmod);
    jit->Optimize(*llmod);

    auto cuda_engine = CudaEngine::Get();
    auto cuda_module = cuda_engine->Build(*llmod);
    return cuda_engine->Lookup(cuda_module, llmod->getName().str());
}
}  // namespace reffine
