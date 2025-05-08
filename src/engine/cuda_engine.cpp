#ifdef ENABLE_CUDA
#include "reffine/engine/cuda_engine.h"

using namespace reffine;
using namespace std::placeholders;
using namespace std;

#define checkCudaErrors(err) __checkCudaErrors(err, __FILE__, __LINE__)
static void __checkCudaErrors(CUresult err, const char* filename, int line)
{
    assert(filename);
    if (CUDA_SUCCESS != err) {
        const char* ename = NULL;
        const CUresult res = cuGetErrorName(err, &ename);
        fprintf(stderr,
                "CUDA API Error %04d: \"%s\" from file <%s>, "
                "line %i.\n",
                err, ((CUDA_SUCCESS == res) ? ename : "Unknown"), filename,
                line);
        exit(err);
    }
}

CudaEngine* CudaEngine::Get()
{
    static unique_ptr<CudaEngine> engine = make_unique<CudaEngine>();

    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    InitializeAllAsmParsers();

    cuInit(0);
    checkCudaErrors(cuDeviceGet(&engine->device, 0));
    checkCudaErrors(cuCtxCreate(&engine->context, 0, engine->device));

    return engine.get();
}

CUfunction CudaEngine::Lookup(CUmodule cudaModule, string kernel_name)
{
    checkCudaErrors(
        cuModuleGetFunction(&function, cudaModule, kernel_name.c_str()));

    return function;
}

CUmodule CudaEngine::Build(Module& llmod)
{
    std::string Error;
    const llvm::Target* Target =
        llvm::TargetRegistry::lookupTarget("nvptx64-nvidia-cuda", Error);

    llvm::TargetOptions opt;
    llvm::TargetMachine* TM =
        Target->createTargetMachine("nvptx64-nvidia-cuda", "sm_75",
                                    "+ptx81",  // PTX version
                                    opt, llvm::Reloc::Static);

    llmod.setDataLayout(TM->createDataLayout());

    llvm::SmallString<1048576> PTXStr;  // max size
    llvm::raw_svector_ostream PTXOS(PTXStr);

    llvm::legacy::PassManager PM;
    TM->addPassesToEmitFile(PM, PTXOS, nullptr,
                            llvm::CodeGenFileType::AssemblyFile);
    PM.run(llmod);

    checkCudaErrors(cuModuleLoadData(&cudaModule, PTXStr.c_str()));

    return cudaModule;
}

void CudaEngine::Cleanup()
{
    cuModuleUnload(cudaModule);
    cuCtxDestroy(context);
}
#endif
