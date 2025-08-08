#ifndef INCLUDE_REFFINE_PASS_CODEGEN_LLVMGEN_H_
#define INCLUDE_REFFINE_PASS_CODEGEN_LLVMGEN_H_

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

using namespace std;

extern const char* vinstr_str;

namespace reffine {

class LLVMGenCtx : public IRPassBaseCtx<llvm::Value*> {
public:
    LLVMGenCtx(shared_ptr<Func> func, map<Sym, llvm::Value*> m = {})
        : IRPassBaseCtx<llvm::Value*>(func->tbl, m)
    {
    }
};

class LLVMGen : public IRGenBase<llvm::Value*> {
public:
    explicit LLVMGen(LLVMGenCtx& ctx, llvm::Module& llmod)
        : IRGenBase(ctx),
          _llmod(llmod),
          _builder(make_unique<llvm::IRBuilder<>>(llmod.getContext()))
    {
        register_vinstrs();
    }

    static void Build(shared_ptr<Func>, llvm::Module&);

private:
    void register_vinstrs();

    llvm::Value* visit(Sym) final;
    llvm::Value* visit(Call&) final;
    llvm::Value* visit(IfElse&) final;
    llvm::Value* visit(NoOp&) final;
    llvm::Value* visit(Select&) final;
    llvm::Value* visit(Const&) final;
    llvm::Value* visit(Get&) final;
    llvm::Value* visit(Cast&) final;
    llvm::Value* visit(NaryExpr&) final;
    llvm::Value* visit(Stmts&) final;
    llvm::Value* visit(Alloc&) final;
    llvm::Value* visit(Load&) final;
    llvm::Value* visit(Store&) final;
    llvm::Value* visit(AtomicOp&) final;
    llvm::Value* visit(StructGEP&) final;
    llvm::Value* visit(ThreadIdx&) final;
    llvm::Value* visit(BlockIdx&) final;
    llvm::Value* visit(BlockDim&) final;
    llvm::Value* visit(GridDim&) final;
    llvm::Value* visit(Loop&) final;
    llvm::Value* visit(FetchDataPtr&) final;
    void visit(Func&) final;

    llvm::Function* llfunc(const string, llvm::Type*, vector<llvm::Type*>);
    llvm::Value* llcall(const string, llvm::Type*, vector<llvm::Value*>);
    llvm::Value* llcall(const string, llvm::Type*, vector<Expr>);

    llvm::Type* lltype(const DataType&);
    llvm::Type* lltype(const ExprNode& expr) { return lltype(expr.type); }
    llvm::Type* lltype(const Expr& expr) { return lltype(expr->type); }

    llvm::Module* llmod() { return &_llmod; }
    llvm::LLVMContext& llctx() { return llmod()->getContext(); }
    llvm::IRBuilder<>* builder() { return _builder.get(); }

    llvm::Module& _llmod;
    unique_ptr<llvm::IRBuilder<>> _builder;

    // Helpers
    llvm::LoadInst* CreateLoad(llvm::Type*, llvm::Value*);
    llvm::StoreInst* CreateStore(llvm::Value*, llvm::Value*);
    llvm::AllocaInst* CreateAlloca(llvm::Type*,
                                   llvm::Value* size = (llvm::Value*)nullptr);
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_CODEGEN_LLVMGEN_H_
