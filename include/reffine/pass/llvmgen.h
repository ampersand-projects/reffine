#ifndef INCLUDE_REFFINE_PASS_CODEGEN_LLVMGEN_H_
#define INCLUDE_REFFINE_PASS_CODEGEN_LLVMGEN_H_

#include <memory>
#include <utility>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>

#include "reffine/pass/irgen.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Linker/Linker.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace std;

extern const char* vinstr_str;

namespace reffine {

class LLVMGenCtx : public IRGenCtx<llvm::Value*> {
public:
    LLVMGenCtx(shared_ptr<Func> func) :
        IRGenCtx(func->tbl)
    {}

private:
    friend class LLVMGen;
};

class LLVMGen : public IRGen<LLVMGenCtx, llvm::Value*> {
public:
    explicit LLVMGen(LLVMGenCtx llgenctx, llvm::Module& llmod) :
        _ctx(std::move(llgenctx)),
        _llmod(llmod),
        _builder(make_unique<llvm::IRBuilder<>>(llmod.getContext()))
    {
        register_vinstrs();
    }

    static void Build(shared_ptr<Func>, llvm::Module&);

private:
    LLVMGenCtx& ctx() override { return _ctx; }

    void register_vinstrs();

    void assign(Sym sym, llvm::Value* val) override;

    llvm::Value* visit(Call&) final;
    void visit(IfElse&) final;
    void visit(NoOp&) final;
    llvm::Value* visit(Select&) final;
    llvm::Value* visit(Exists&) final;
    llvm::Value* visit(Const&) final;
    llvm::Value* visit(Cast&) final;
    llvm::Value* visit(NaryExpr&) final;
    llvm::Value* visit(Read&) final { throw runtime_error("Operation not supported"); }
    llvm::Value* visit(Write&) final { throw runtime_error("Operation not supported"); }
    void visit(Stmts&) final;
    void visit(Func&) final;
    llvm::Value* visit(Alloc&) final;
    llvm::Value* visit(Load&) final;
    void visit(Store&) final;
    llvm::Value* visit(Loop&) final;
    llvm::Value* visit(IsValid&) final;
    llvm::Value* visit(SetValid&) final;
    llvm::Value* visit(FetchDataPtr&) final;

    llvm::Function* llfunc(const string, llvm::Type*, vector<llvm::Type*>);
    llvm::Value* llcall(const string, llvm::Type*, vector<llvm::Value*>);
    llvm::Value* llcall(const string, llvm::Type*, vector<Expr>);

    llvm::Type* lltype(const DataType&);
    llvm::Type* lltype(const ExprNode& expr) { return lltype(expr.type); }
    llvm::Type* lltype(const Expr& expr) { return lltype(expr->type); }

    llvm::Module* llmod() { return &_llmod; }
    llvm::LLVMContext& llctx() { return llmod()->getContext(); }
    llvm::IRBuilder<>* builder() { return _builder.get(); }

    LLVMGenCtx _ctx;
    llvm::Module& _llmod;
    unique_ptr<llvm::IRBuilder<>> _builder;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_CODEGEN_LLVMGEN_H_
