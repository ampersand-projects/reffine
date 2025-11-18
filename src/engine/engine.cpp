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
#include "reffine/vinstr/vinstr.h"

using namespace reffine;
using namespace std::placeholders;

ExecEngine* ExecEngine::Get()
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
    raw_fd_ostream r(fileno(stderr), false);
    if (verifyModule(*m, &r)) {
        throw std::runtime_error("LLVM module verification failed!!!");
    } else {
        cantFail(optimizer.add(jd, ThreadSafeModule(std::move(m), ctx)));
    }
}

LLVMContext& ExecEngine::GetCtx()
{
    LLVMContext* contextPtr = nullptr;
    ctx.withContextDo([&](LLVMContext* context) { contextPtr = context; });
    return *contextPtr;
}

Expected<ThreadSafeModule> ExecEngine::optimize_module(
    ThreadSafeModule tsm, const MaterializationResponsibility& r)
{
    tsm.withModuleDo([](Module& m) {
        auto fpm = std::make_unique<legacy::FunctionPassManager>(&m);
        fpm->add(createInstructionCombiningPass());
        fpm->add(createReassociatePass());
        fpm->add(createGVNPass());
        fpm->add(createCFGSimplificationPass());
        fpm->doInitialization();

        for (auto& f : m) { fpm->run(f); }
    });

    return std::move(tsm);
}

void ExecEngine::Optimize(Module& llmod) { MPM.run(llmod, MAM); }

unique_ptr<ExecutionSession> ExecEngine::createExecutionSession()
{
    unique_ptr<SelfExecutorProcessControl> epc =
        llvm::cantFail(SelfExecutorProcessControl::Create());
    return std::make_unique<ExecutionSession>(std::move(epc));
}

void ExecEngine::register_symbols()
{
    cantFail(jd.define(absoluteSymbols(SymbolMap(
        {{mangler("make_vector"),
          {ExecutorAddr::fromPtr(&make_vector), JITSymbolFlags::Callable}},
         {mangler("vector_locate"),
          {ExecutorAddr::fromPtr(&vector_locate),
           JITSymbolFlags::Callable}}}))));
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
