#ifndef INCLUDE_REFFINE_ENGINE_ENGINE_H_
#define INCLUDE_REFFINE_ENGINE_ENGINE_H_

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
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/TargetSelect.h"

using namespace std;
using namespace llvm;
using namespace llvm::orc;

namespace reffine {

class ExecEngine {
public:
    ExecEngine(JITTargetMachineBuilder jtmb, DataLayout dl)
        : es(createExecutionSession()),
          linker(*es, []() { return make_unique<SectionMemoryManager>(); }),
          compiler(*es, linker,
                   make_unique<ConcurrentIRCompiler>(std::move(jtmb))),
          optimizer(*es, compiler, optimize_module),
          dl(std::move(dl)),
          mangler(*es, this->dl),
          ctx(make_unique<LLVMContext>()),
          jd(es->createBareJITDylib("__reffine_dylib"))
    {
        jd.addGenerator(
            cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
                dl.getGlobalPrefix())));

        register_symbols();
        add_opt_passes();
    }

    static ExecEngine* Get();
    void Optimize(Module&);
    void GeneratePTX(Module&, std::string&);
    void ExecutePTX(const std::string&, const std::string&, void*, int*);
    void ExecutePTXTest(const std::string&, const std::string&, void*, int*);
    void AddModule(unique_ptr<Module>);
    LLVMContext& GetCtx();

    template <typename FnTy>
    FnTy Lookup(StringRef name)
    {
        auto fn_sym = cantFail(es->lookup({&jd}, mangler(name.str())));
        return fn_sym.getAddress().toPtr<FnTy>();
    }

private:
    static Expected<ThreadSafeModule> optimize_module(
        ThreadSafeModule, const MaterializationResponsibility&);
    static unique_ptr<ExecutionSession> createExecutionSession();

    unique_ptr<ExecutionSession> es;
    RTDyldObjectLinkingLayer linker;
    IRCompileLayer compiler;
    IRTransformLayer optimizer;

    DataLayout dl;
    MangleAndInterner mangler;
    ThreadSafeContext ctx;

    JITDylib& jd;

    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;
    PassBuilder PB;
    FunctionPassManager FPM;
    LoopPassManager LM;
    ModulePassManager MPM;

    void register_symbols();
    void add_opt_passes();
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ENGINE_ENGINE_H_
