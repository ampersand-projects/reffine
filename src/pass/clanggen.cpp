#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "llvm/Support/MemoryBuffer.h"

#include "reffine/pass/clanggen.h"

using namespace std;
using namespace llvm;
using namespace reffine;

unique_ptr<Module> ClangGen::Build(shared_ptr<Func> func)
{
    string code = R"(
        extern "C" int add(int a, int b) {
            return a + b;
        }
    )";

    auto buffer = llvm::MemoryBuffer::getMemBuffer(code, "in_memory.c");

    // 2. Set up a Clang CompilerInstance
    clang::CompilerInstance CI;
    CI.createDiagnostics();

    auto invocation = std::make_shared<clang::CompilerInvocation>();
    clang::CompilerInvocation::CreateFromArgs(
        *invocation,
        {"-xc", "-O2", "-emit-llvm", "-"},
        CI.getDiagnostics());

    CI.setInvocation(invocation);

    // 3. Emit LLVM IR from the input buffer
    clang::EmitLLVMOnlyAction act;
    if (!CI.ExecuteAction(act)) {
        llvm::errs() << "Failed to compile\n";
        return nullptr;
    }
    auto llmod = act.takeModule();

    return llmod;
}
