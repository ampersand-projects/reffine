#ifndef INCLUDE_REFFINE_PASS_CODEGEN_CUDAGEN_H_
#define INCLUDE_REFFINE_PASS_CODEGEN_CUDAGEN_H_

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "reffine/pass/llvmgen.h"

using namespace std;

namespace reffine {

class CUDAGenCtx : public LLVMGenCtx{
public:
    CUDAGenCtx(shared_ptr<Func> func, map<Sym, llvm::Value*> m = {})
        : LLVMGenCtx(func, m)
    {
    }
};


class CUDAGen : public LLVMGen {
    public:
        explicit CUDAGen(LLVMGenCtx& ctx, llvm::Module& llmod)
            : LLVMGen(ctx, llmod) //,
            //   _llmod(llmod),
            //   _builder(make_unique<llvm::IRBuilder<>>(llmod.getContext()))
        {
        }
    
        static void Build(shared_ptr<Func>, llvm::Module&);


    private:
        llvm::Value* visit(ThreadIdx&) final;
        llvm::Value* visit(BlockIdx&) final;
        llvm::Value* visit(BlockDim&) final;
        llvm::Value* visit(GridDim&) final;
        void visit(Func&) final;
};
}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_CODEGEN_CUDAGEN_H_