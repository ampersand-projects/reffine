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
// #include <cuda_runtime.h>

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

void ExecEngine::GeneratePTX(Module &llmod)
{
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    InitializeAllAsmParsers();

    llmod.setTargetTriple("nvptx64-nvidia-cuda");

    // std::string error;
    // auto target = llvm::TargetRegistry::lookupTarget(llmod.getTargetTriple(),
    // error);
    // llvm::TargetRegistry::lookupTarget(llmod.getTargetTriple(), error);

    // llvm::TargetOptions opt;
    // auto targetMachine = target->createTargetMachine(llmod.getTargetTriple(),
    // "sm_52", "", opt, llvm::Reloc::PIC_);
    // llmod.setDataLayout(targetMachine->createDataLayout());

    // std::error_code ec;
    // llvm::raw_fd_ostream dest("output.ptx", ec, llvm::sys::fs::OF_None);

    // llvm::legacy::PassManager pass;
    // if (targetMachine->addPassesToEmitFile(pass, dest, nullptr,
    // llvm::LLVMAssemblyFile)) {
    //     llvm::errs() << "TargetMachine can't emit a file of this type";
    //     return;
    // }

    // pass.run(llmod);
    // dest.flush();
}

void ExecEngine::GeneratePTX(Module &llmod, std::string& outputPTX, std::string computeCapability)
{
    // Initialize NVPTX target
    std::string Error;
    const llvm::Target *Target = llvm::TargetRegistry::lookupTarget("nvptx64-nvidia-cuda", Error);
    
    if (!Target) {
        cout << "Error" << Error << endl;
        return;
    }
    
    // Create target machine
    llvm::TargetOptions Opt;
    llvm::TargetMachine *TM = Target->createTargetMachine(
        "nvptx64-nvidia-cuda",
        computeCapability,  // e.g., "sm_75"
        "+ptx63",          // PTX version
        Opt,
        llvm::Reloc::Static);
    
    // Set data layout
    llmod.setDataLayout(TM->createDataLayout());
    
    // Generate PTX
    llvm::SmallString<1024> PTXStr;
    llvm::raw_svector_ostream PTXOS(PTXStr);
    
    llvm::legacy::PassManager PM;
    TM->addPassesToEmitFile(PM, PTXOS, nullptr, llvm::CodeGenFileType::AssemblyFile);
    PM.run(llmod);

    std::cout << "Generated PTXStr: " << std::endl << PTXStr.str().str() << std::endl;
    
    outputPTX = PTXStr.str().str();
}

void ExecEngine::ExecutePTX(const std::string& ptxCode, const std::string& kernel_name) {
    CUdevice device;
    CUmodule cudaModule;
    CUcontext context;
    CUfunction function;
    // CUlinkState linker;
    
    cuInit(0);
    cuDeviceGet(&device, 0);
    cuCtxCreate(&context, 0, device);

    char name[128];
    cuDeviceGetName(name, 128, device);
    std::cout << "Using CUDA Device [0]: " << name << "\n";

    cuModuleLoadData(&cudaModule, ptxCode.c_str());

    cuModuleGetFunction(&function, cudaModule, kernel_name.c_str());

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
