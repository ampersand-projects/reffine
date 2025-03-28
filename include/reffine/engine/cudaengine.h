#ifndef INCLUDE_REFFINE_CUDA_ENGINE_H_
#define INCLUDE_REFFINE_CUDA_ENGINE_H_

#include <memory>
#include <utility>

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"

using namespace std;
using namespace llvm;
using namespace llvm::orc;

namespace reffine {

class CUDAEngine {
public:
    CUDAEngine(Module& llmod) : llmod(llmod) {}

    static CUDAEngine* Init(Module& llmod);
    void GeneratePTX();
    void ExecutePTX(void*, int*);
    void PrintPTX();
    string GetKernelName();

    Module& llmod;

private:
    string kernel_name;
    string ptx_str;

    TargetMachine* get_target();
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_CUDA_ENGINE_H_
