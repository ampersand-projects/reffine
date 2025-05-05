// #ifdef ENABLE_CUDA
#include "reffine/engine/cuda_engine.h"

using namespace reffine;
using namespace std::placeholders;
using namespace std;

CudaEngine* CudaEngine::Get()
{
    static unique_ptr<CudaEngine> engine = make_unique<CudaEngine>();

    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    InitializeAllAsmParsers();

    CUdevice device;
    // CUmodule cudaModule;
    CUcontext context;
    // CUfunction function;

    cuInit(0);
    cuDeviceGet(&device, 0);
    cuCtxCreate(&context, 0, device);

    return engine.get();
}

void CudaEngine::AddModule(unique_ptr<Module> m)
{
    // ...
}

// LLVMContext& CudaEngine::GetCtx() { 
    // ...
// }

TargetMachine* CudaEngine::get_target(Module& llmod)
{
    std::string Error;
    const llvm::Target* Target =
        llvm::TargetRegistry::lookupTarget("nvptx64-nvidia-cuda", Error);

    llvm::TargetOptions opt;
    llvm::TargetMachine* TM =
        Target->createTargetMachine("nvptx64-nvidia-cuda", "sm_52",
                                    "+ptx76",  // PTX version
                                    opt, llvm::Reloc::Static);

    llmod.setDataLayout(TM->createDataLayout());

    return TM;
}

CUfunction CudaEngine::Lookup(CUmodule cudaModule, string kernel_name) {
    CUfunction function;
    cuModuleGetFunction(&function, cudaModule, kernel_name.c_str());

    return function;
}

CUmodule CudaEngine::Build(Module& llmod)
{
    std::string Error;
    const llvm::Target* Target =
        llvm::TargetRegistry::lookupTarget("nvptx64-nvidia-cuda", Error);

    llvm::TargetOptions opt;
    llvm::TargetMachine* TM =
        Target->createTargetMachine("nvptx64-nvidia-cuda", "sm_52",
                                    "+ptx76",  // PTX version
                                    opt, llvm::Reloc::Static);

    llmod.setDataLayout(TM->createDataLayout());

    llvm::SmallString<1048576> PTXStr;
    llvm::raw_svector_ostream PTXOS(PTXStr);

    llvm::legacy::PassManager PM;
    TM->addPassesToEmitFile(PM, PTXOS, nullptr,
                            llvm::CodeGenFileType::AssemblyFile);
    PM.run(llmod);

    // std::cout << "Generated PTX:" << endl << PTXStr << endl;

    CUmodule cudaModule;
    cuModuleLoadData(&cudaModule, PTXStr.c_str());

    return cudaModule;
}
// #endif