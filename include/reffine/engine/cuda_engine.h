#ifdef ENABLE_CUDA

#ifndef INCLUDE_REFFINE_CUDA_ENGINE_ENGINE_H_
#define INCLUDE_REFFINE_CUDA_ENGINE_ENGINE_H_

#include <memory>
#include <utility>

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

namespace reffine {

class CudaEngine {
public:
    CudaEngine() {}

    static CudaEngine* Get();
    CUmodule Build(Module&);
    CUfunction Lookup(CUmodule, string);
    void Cleanup();

    CUdevice device;
    CUcontext context;
    CUmodule cudaModule;
    CUfunction function;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_CUDA_ENGINE_ENGINE_H_
#endif  // ENABLE_CUDA