#ifndef INCLUDE_REFFINE_PASS_CODEGEN_LLVMGEN_H_
#define INCLUDE_REFFINE_PASS_CODEGEN_LLVMGEN_H_

#include <memory>
#include <utility>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>

#include "reffine/pass/irpass.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Linker/Linker.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace std;

namespace reffine {

class LLVMGenCtx : public IRPassCtx<llvm::Value*> {
public:
    LLVMGenCtx(const shared_ptr<Func> func) :
        IRPassCtx(func->tbl)
    {}

private:
    friend class LLVMGen;
};

class LLVMGen : public IRPass<LLVMGenCtx, llvm::Value*> {
public:
    explicit LLVMGen(LLVMGenCtx llgenctx, llvm::Module& llmod) :
        _ctx(std::move(llgenctx)),
        _llmod(llmod),
        _builder(make_unique<llvm::IRBuilder<>>(llmod.getContext()))
    {}

    static void Build(const shared_ptr<Func>, llvm::Module&);

private:
    LLVMGenCtx& ctx() override { return _ctx; }

    void assign(Sym sym, llvm::Value* val) override
    {
        IRPass::assign(sym, val);
        val->setName(sym->name);
    }

    llvm::Value* visit(const Call&) final;
    llvm::Value* visit(const IfElse&) final;
    llvm::Value* visit(const Select&) final;
    llvm::Value* visit(const Exists&) final;
    llvm::Value* visit(const Const&) final;
    llvm::Value* visit(const Cast&) final;
    llvm::Value* visit(const NaryExpr&) final;
    llvm::Value* visit(const Read&) final;
    llvm::Value* visit(const PushBack&) final;
    llvm::Value* visit(const Stmts&) final;
    llvm::Value* visit(const Func&) final;
    llvm::Value* visit(const Assign&) final;
    llvm::Value* visit(const Loop&) final;

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
