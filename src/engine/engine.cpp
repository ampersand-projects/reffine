#include <iostream>

#include "reffine/engine/engine.h"

#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/IPO/PartialInlining.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/ADCE.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/InstSimplifyPass.h"
#include "llvm/Transforms/Scalar/LoopRotation.h"
#include "llvm/Transforms/Scalar/LoopUnrollPass.h"
#include "llvm/Transforms/Scalar/SCCP.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Scalar/Sink.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Vectorize/LoopVectorize.h"
#include "llvm/Transforms/Vectorize/SLPVectorizer.h"
#include "llvm/MC/TargetRegistry.h"
#include <cuda.h>
#include <cassert>
#include <cuda_runtime.h>

using namespace reffine;
using namespace std::placeholders;

ExecEngine *ExecEngine::Get()
{
    static unique_ptr<ExecEngine> engine;

    if (!engine) {
        InitializeNativeTarget();
        InitializeNativeTargetAsmPrinter();

        auto jtmb = cantFail(JITTargetMachineBuilder::detectHost());
        auto dl = cantFail(jtmb.getDefaultDataLayoutForTarget());

        engine = make_unique<ExecEngine>(std::move(jtmb), std::move(dl));
    }

    return engine.get();
}

void ExecEngine::AddModule(unique_ptr<Module> m)
{
    raw_fd_ostream r(fileno(stdout), false);
    verifyModule(*m, &r);

    cantFail(optimizer.add(jd, ThreadSafeModule(std::move(m), ctx)));
}

LLVMContext &ExecEngine::GetCtx() { return *ctx.getContext(); }

Expected<ThreadSafeModule> ExecEngine::optimize_module(
    ThreadSafeModule tsm, const MaterializationResponsibility &r)
{
    tsm.withModuleDo([](Module &m) {
        auto fpm = std::make_unique<legacy::FunctionPassManager>(&m);
        fpm->add(createInstructionCombiningPass());
        fpm->add(createReassociatePass());
        fpm->add(createGVNPass());
        fpm->add(createCFGSimplificationPass());
        fpm->doInitialization();

        for (auto &f : m) { fpm->run(f); }
    });

    return std::move(tsm);
}

void ExecEngine::Optimize(Module &llmod) { MPM.run(llmod, MAM); }

void ExecEngine::GeneratePTX(Module &llmod, std::string& outputPTX)
{
    // setup and initialization
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    InitializeAllAsmParsers();

    llmod.setTargetTriple("nvptx64-nvidia-cuda");

    std::string Error;
    const llvm::Target *Target = llvm::TargetRegistry::lookupTarget("nvptx64-nvidia-cuda", Error);
    
    if (!Target) {
        cout << "Error" << Error << endl;
        return;
    }

    llvm::TargetOptions opt;
    llvm::TargetMachine *TM = Target->createTargetMachine(
        "nvptx64-nvidia-cuda",  // checked by running `$ llc --version`
        "sm_52",        // for NVIDIA GeForce RTX 2080 Ti , checked by running `$ nvidia-smi`
        "+ptx73",     //"+ptx76",         // PTX version
        // "",     // "+ptx63",          
        opt,
        llvm::Reloc::Static);
    
    llmod.setDataLayout(TM->createDataLayout());

    cout << "Created data layout" << endl;
    
    // generate PTX
    llvm::SmallString<1048576> PTXStr;
    llvm::raw_svector_ostream PTXOS(PTXStr);
    
    llvm::legacy::PassManager PM;
    TM->addPassesToEmitFile(PM, PTXOS, nullptr, llvm::CodeGenFileType::AssemblyFile);
    PM.run(llmod);

    cout << "Run PTX generation pass" << endl;

    // std::cout << "Generated PTXStr: " << std::endl << PTXStr.str().str() << std::endl;
    
    outputPTX = PTXStr.str().str();
}

// static void checkCudaErrors(CUresult err)
// {
//     assert(err == CUDA_SUCCESS);
// }
#define checkCudaErrors(err) __checkCudaErrors(err, __FILE__, __LINE__)
static void __checkCudaErrors(CUresult err, const char *filename, int line) {
  assert(filename);
  if (CUDA_SUCCESS != err) {
    const char *ename = NULL;
    const CUresult res = cuGetErrorName(err, &ename);
    fprintf(stderr,
            "CUDA API Error %04d: \"%s\" from file <%s>, "
            "line %i.\n",
            err, ((CUDA_SUCCESS == res) ? ename : "Unknown"), filename, line);
    exit(err);
  }
}


void ExecEngine::ExecutePTX(const std::string& ptxCode, const std::string& kernel_name, void* arg, int* result) {
    CUdevice device;
    CUmodule cudaModule;
    CUcontext context;
    CUfunction function;
    // CUlinkState linker;
    
    checkCudaErrors(cuInit(0));
    checkCudaErrors(cuDeviceGet(&device, 0));
    checkCudaErrors(cuCtxCreate(&context, 0, device));
    if (!context) {
        std::cerr << "CUDA context is not set!" << std::endl;
    }

    char name[128];
    cuDeviceGetName(name, 128, device);
    std::cout << "Device name: " << name << "\n";

    CUdeviceptr d_result;
    checkCudaErrors(cuMemAlloc(&d_result, sizeof(int64_t)));

    CUdeviceptr d_idx;
    checkCudaErrors(cuMemAlloc(&d_idx, sizeof(int64_t)));
 
    int len = 1024;
    CUdeviceptr d_arr;
    checkCudaErrors(cuMemAlloc(&d_arr, sizeof(int64_t)*len));
    checkCudaErrors(cuMemcpyHtoD(d_arr, arg, sizeof(int64_t)*len));

    CUdeviceptr d_arr_out;
    checkCudaErrors(cuMemAlloc(&d_arr_out, sizeof(int64_t)*len));

    // cout << "Generated PTX 2:" << endl << ptxCode.c_str() << endl;
    // checkCudaErrors(cuModuleLoadData(&cudaModule, ptxCode.c_str()));

    setenv("CUDA_LAUNCH_BLOCKING", "1", 1);

    // Use more detailed error checking
    checkCudaErrors(cuModuleLoadData(&cudaModule, ptxCode.c_str()));
    cout << "cuModuleLoadData passed!! loaded from ptxCode param" << endl;

    cout << kernel_name.c_str() << endl;
    checkCudaErrors(cuModuleGetFunction(&function, cudaModule, kernel_name.c_str()));
    cout << "cuModuleGetFunction passed!! running " << kernel_name << endl;

    // gridDim: num of blocks 
    // blockDim: num of threads in block
    int blockDimX = 32;
    // int gridDimX = (len + blockDimX - 1) / blockDimX;
    int gridDimX = 4;
    void* kernelParams[] = { 
        &d_arr, 
        // &d_result,
        &d_arr_out,
        // &d_idx
    };

    checkCudaErrors(cuLaunchKernel(function,
        gridDimX, 1, 1,
        blockDimX, 1, 1,
        0,   // shared memory size
        0,   // stream handle
        kernelParams,
        NULL)); 
    
    checkCudaErrors(cuCtxSynchronize());
    cout << "cuCtxSynchronize passed!! " << endl;

    checkCudaErrors(cuMemcpyDtoH(result, d_result, sizeof(int64_t)));
    cout << "Result copied cuMemcpyDtoH!! " << endl;
    cout << "Kernel result: " << *result << endl;
    cuMemFree(d_result);

    auto arr_out = (int64_t*)malloc(sizeof(int64_t)*len);
    checkCudaErrors(cuMemcpyDtoH(arr_out, d_arr_out, sizeof(int64_t)*len));
    cout << "Output array from kernel:" << endl;
    for (int i = 0; i < len; i++) {
        cout << arr_out[i] << ", ";
    }
    cout << endl;

    cuModuleUnload(cudaModule);
    cuCtxDestroy(context);
}

#include <fstream>
void ExecEngine::ExecutePTXFromFile(const std::string& ptxCode, const std::string& kernel_name, void* arg, int* res) {

    CUdevice device;
    CUmodule cudaModule;
    CUcontext context;
    CUfunction function;
    // CUlinkState linker;

    // Generated PTX from CUDA vector_fn.cu
    // char file_name[] = "../vector_kernel.ptx";
    // char fcn_kernel_name[] = "_Z9vector_fniPiS_";

    // Generated PTX from CUDA test_fn.cu
    // char file_name[] = "../test_fn.ptx";
    // char fcn_kernel_name[] = "_Z9vector_fnPi";

    // Generated PTX from vector_fn_3 in main.cpp
        // runs as long as you comment out retval line 24
        // //st.param.b64    [func_retval0+0], %rd5;
    char file_name[] = "../vector_fn_3_kernel.ptx";
    char fcn_kernel_name[] = "foo";

    
    checkCudaErrors(cuInit(0));
    checkCudaErrors(cuDeviceGet(&device, 0));
    checkCudaErrors(cuCtxCreate(&context, 0, device));
    if (!context) {
        std::cerr << "CUDA context is not set!" << std::endl;
    }

    char name[128];
    cuDeviceGetName(name, 128, device);
    std::cout << "Device name: " << name << "\n";

    // CUdeviceptr d_result;
    // checkCudaErrors(cuMemAlloc(&d_result, sizeof(int)));
 
    // CUdeviceptr d_arr;
    // checkCudaErrors(cuMemAlloc(&d_arr, sizeof(int64_t)*100));
    // checkCudaErrors(cuMemcpyHtoD(d_arr, arg, sizeof(int64_t)*100));
    

    // cout << "Generated PTX 2:" << endl << ptxCode.c_str() << endl;
    // checkCudaErrors(cuModuleLoadData(&cudaModule, ptxCode.c_str()));

    setenv("CUDA_LAUNCH_BLOCKING", "1", 1);

    ifstream MyReadFile(file_name);
    string myText;
    while (getline(MyReadFile, myText)) {
        // Output the text from the file
        cout << myText << endl;
    }
    MyReadFile.close();
    // return;

    // Use more detailed error checking
    checkCudaErrors(cuModuleLoad(&cudaModule, file_name));

    cout << "cuModuleLoad passed!! loaded from " << file_name << endl;

    int result = 0;
    // int len = 100;
    CUdeviceptr d_result;
    checkCudaErrors(cuMemAlloc(&d_result, sizeof(int64_t)));
    checkCudaErrors(cuMemcpyHtoD(d_result, &result, sizeof(int64_t)));
 
    CUdeviceptr d_arr;
    checkCudaErrors(cuMemAlloc(&d_arr, sizeof(int64_t)*100));
    checkCudaErrors(cuMemcpyHtoD(d_arr, arg, sizeof(int64_t)*100));

    void* kernelParams[] = {
        // &len,
        &d_arr, 
        &d_result
    };

    checkCudaErrors(cuModuleGetFunction(&function, cudaModule, fcn_kernel_name));
    cout << "cuModuleGetFunction passed!! running " << fcn_kernel_name << endl;
    checkCudaErrors(cuLaunchKernel(function,
        1, 1, 1,    // Grid dimensions
        1, 1, 1,    // Block dimensions
        0,          // Shared memory size
        0,          // Stream
        kernelParams,
        NULL));

    checkCudaErrors(cuMemcpyDtoH(&result, d_result, sizeof(int)));
    std::cout << "Kernel Result: " << result << std::endl;
}



unique_ptr<ExecutionSession> ExecEngine::createExecutionSession()
{
    unique_ptr<SelfExecutorProcessControl> epc =
        llvm::cantFail(SelfExecutorProcessControl::Create());
    return std::make_unique<ExecutionSession>(std::move(epc));
}

void ExecEngine::register_symbols()
{
    cantFail(jd.define(absoluteSymbols(SymbolMap({
        //{ mangler("get_vector_len"), { ExecutorAddr::fromPtr(&get_vector_len),
        // JITSymbolFlags::Callable } }
    }))));
}

void ExecEngine::add_opt_passes()
{
    // based on optimization pipeline here:
    // https://github.com/csb6/bluebird/blob/master/src/optimizer.cpp
    VectorizerParams::VectorizationFactor = 4;

    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    FPM.addPass(SCCPPass());
    FPM.addPass(ADCEPass());
    FPM.addPass(
        LoopSimplifyPass());  // needs to be followed by a simplifycfg pass
    {
        LM.addPass(LoopRotatePass());
        FPM.addPass(createFunctionToLoopPassAdaptor(std::move(LM)));
    }
    FPM.addPass(LoopUnrollPass());
    FPM.addPass(SimplifyCFGPass());
    FPM.addPass(ADCEPass());
    FPM.addPass(SinkingPass());
    FPM.addPass(LoopVectorizePass());
    FPM.addPass(SLPVectorizerPass());

    MPM = PB.buildPerModuleDefaultPipeline(OptimizationLevel::O3);
    MPM.addPass(PartialInlinerPass());
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
}
