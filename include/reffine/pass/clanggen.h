#ifndef INCLUDE_REFFINE_PASS_CODEGEN_CLANGGEN_H_
#define INCLUDE_REFFINE_PASS_CODEGEN_CLANGGEN_H_

#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "reffine/pass/base/irgen.h"

namespace reffine {

class ClangGen {
public:
    static std::unique_ptr<llvm::Module> Build(shared_ptr<Func>);

private:
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_CODEGEN_CLANGGEN_H_
