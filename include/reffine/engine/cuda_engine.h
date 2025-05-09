#ifdef ENABLE_CUDA

#ifndef INCLUDE_REFFINE_CUDA_ENGINE_ENGINE_H_
#define INCLUDE_REFFINE_CUDA_ENGINE_ENGINE_H_

#include <cuda.h>
#include <cuda_runtime.h>

#include <memory>
#include <utility>

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

using namespace std;
using namespace llvm;

namespace reffine {

class CudaEngine {
public:
    CudaEngine() {}

    static unique_ptr<CudaEngine> engine;

    static CudaEngine* Get();
    CUmodule Build(Module&);
    CUfunction Lookup(CUmodule, string);
    void Cleanup(CUmodule);

    CUdevice device;
    CUcontext context;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_CUDA_ENGINE_ENGINE_H_
#endif  // ENABLE_CUDA
