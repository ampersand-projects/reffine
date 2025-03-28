#include "reffine/engine/cudaengine.h"

#include <cuda.h>
#include <cuda_runtime.h>

#include <cassert>
#include <iostream>

#include "llvm/MC/TargetRegistry.h"

using namespace reffine;
using namespace std;
using namespace std::placeholders;

CUDAEngine *CUDAEngine::Init(Module &llmod)
{
    static unique_ptr<CUDAEngine> engine;

    if (!engine) {
        engine = make_unique<CUDAEngine>(llmod);

        InitializeAllTargets();
        InitializeAllTargetMCs();
        InitializeAllAsmPrinters();
        InitializeAllAsmParsers();

        llmod.setTargetTriple("nvptx64-nvidia-cuda");
        engine->kernel_name = llmod.getName().str();
    }

    return engine.get();
}

#define checkCudaErrors(err) __checkCudaErrors(err, __FILE__, __LINE__)
static void __checkCudaErrors(CUresult err, const char *filename, int line)
{
    assert(filename);
    if (CUDA_SUCCESS != err) {
        const char *ename = NULL;
        const CUresult res = cuGetErrorName(err, &ename);
        fprintf(stderr,
                "CUDA API Error %04d: \"%s\" from file <%s>, "
                "line %i.\n",
                err, ((CUDA_SUCCESS == res) ? ename : "Unknown"), filename,
                line);
        exit(err);
    }
}

TargetMachine *CUDAEngine::get_target()
{
    std::string Error;
    const llvm::Target *Target =
        llvm::TargetRegistry::lookupTarget("nvptx64-nvidia-cuda", Error);

    llvm::TargetOptions opt;
    llvm::TargetMachine *TM = Target->createTargetMachine(
        "nvptx64-nvidia-cuda",  // checked by running `$ llc --version`
        "sm_52",   // for NVIDIA GeForce RTX 2080 Ti , checked by running `$
                   // nvidia-smi`
        "+ptx73",  //"+ptx76",         // PTX version
        opt, llvm::Reloc::Static);

    llmod.setDataLayout(TM->createDataLayout());

    return TM;
}

void CUDAEngine::GeneratePTX()
{
    auto TM = get_target();

    llvm::SmallString<1048576> PTXStr;
    llvm::raw_svector_ostream PTXOS(PTXStr);

    llvm::legacy::PassManager PM;
    TM->addPassesToEmitFile(PM, PTXOS, nullptr,
                            llvm::CodeGenFileType::AssemblyFile);
    PM.run(llmod);

    cout << "Finished generating PTX..." << endl;
    ptx_str = PTXStr.str().str();
}

void CUDAEngine::ExecutePTX(void *arg, int *result)
{
    CUdevice device;
    CUmodule cudaModule;
    CUcontext context;
    CUfunction function;

    checkCudaErrors(cuInit(0));
    checkCudaErrors(cuDeviceGet(&device, 0));
    checkCudaErrors(cuCtxCreate(&context, 0, device));

    char name[128];
    cuDeviceGetName(name, 128, device);
    std::cout << "Device name: " << name << "\n";

    int len = 1024;
    CUdeviceptr d_arr;
    checkCudaErrors(cuMemAlloc(&d_arr, sizeof(int64_t) * len));
    checkCudaErrors(cuMemcpyHtoD(d_arr, arg, sizeof(int64_t) * len));

    CUdeviceptr d_arr_out;
    checkCudaErrors(cuMemAlloc(&d_arr_out, sizeof(int64_t) * len));

    checkCudaErrors(cuModuleLoadData(&cudaModule, ptx_str.c_str()));
    checkCudaErrors(
        cuModuleGetFunction(&function, cudaModule, kernel_name.c_str()));
    cout << "About to run " << kernel_name << " kernel." << endl;

    int blockDimX = 32;  // num of threads per block
    int gridDimX = (len + blockDimX - 1) / blockDimX;  // num of blocks
    void *kernelParams[] = {
        &d_arr_out,
        &d_arr,
    };

    checkCudaErrors(cuLaunchKernel(function, gridDimX, 1, 1, blockDimX, 1, 1,
                                   0,  // shared memory size
                                   0,  // stream handle
                                   kernelParams, NULL));

    checkCudaErrors(cuCtxSynchronize());

    auto arr_out = (int64_t *)malloc(sizeof(int64_t) * len);
    checkCudaErrors(cuMemcpyDtoH(arr_out, d_arr_out, sizeof(int64_t) * len));
    cuMemFree(d_arr);
    cout << "Output from " << kernel_name << " kernel:" << endl;
    for (int i = 0; i < len; i++) { cout << arr_out[i] << ", "; }
    cout << endl << endl;

    cuMemFree(d_arr);
    cuMemFree(d_arr_out);
    cuModuleUnload(cudaModule);
    cuCtxDestroy(context);
}

void CUDAEngine::PrintPTX()
{
    cout << "Generated PTX:" << endl << ptx_str << endl << endl;
}

std::string CUDAEngine::GetKernelName() { return kernel_name; }
