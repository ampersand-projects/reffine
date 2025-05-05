// #ifdef ENABLE_CUDA

#ifndef INCLUDE_REFFINE_CUDA_ENGINE_ENGINE_H_
#define INCLUDE_REFFINE_CUDA_ENGINE_ENGINE_H_

#include <memory>
#include <utility>

// #include "llvm/ADT/StringRef.h"
// #include "llvm/ExecutionEngine/JITSymbol.h"
// #include "llvm/ExecutionEngine/Orc/CompileUtils.h"
// #include "llvm/ExecutionEngine/Orc/Core.h"
// #include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
// #include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
// #include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
// #include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
// #include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
// #include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/TargetSelect.h"
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetMachine.h>
#include <cuda.h>
#include <cuda_runtime.h>

using namespace std;
using namespace llvm;
// using namespace llvm::orc;

namespace reffine {

class CudaEngine {
public:
    CudaEngine() {}
    //     : es(createExecutionSession()),
    //       linker(*es, []() { return make_unique<SectionMemoryManager>(); }),
    //       compiler(*es, linker,
    //                make_unique<ConcurrentIRCompiler>(std::move(jtmb))),
    //       optimizer(*es, compiler, optimize_module),
    //       dl(std::move(dl)),
    //       mangler(*es, this->dl),
    //       ctx(make_unique<LLVMContext>()),
    //       jd(es->createBareJITDylib("__reffine_dylib"))
    // {
        
    // }

    static CudaEngine* Get();
    void AddModule(unique_ptr<Module>);
    LLVMContext& GetCtx();

    CUmodule Build(Module&);

    CUfunction Lookup(CUmodule, string);

private:
    // DataLayout dl;

    llvm::TargetMachine* get_target(Module&);
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_CUDA_ENGINE_ENGINE_H_
// #endif  // ENABLE_CUDA