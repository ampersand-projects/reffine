#ifndef INCLUDE_REFFINE_PASS_CODEGEN_CLANGGEN_H_
#define INCLUDE_REFFINE_PASS_CODEGEN_CLANGGEN_H_

#include <string>

#include "llvm/IR/Module.h"
#include "reffine/pass/base/irgen.h"

namespace reffine {

struct CodeSegment {
    virtual std::string to_string(int) = 0;
    virtual ~CodeSegment() {}
};
typedef std::shared_ptr<CodeSegment> CodeSeg;

struct StrSegment : public CodeSegment {
};
typedef std::shared_ptr<StrSegment> StrSeg;

struct LineSegment : public StrSegment {
};
typedef std::shared_ptr<LineSegment> LineSeg;

struct BlockSegment : public CodeSegment {
};
typedef std::shared_ptr<BlockSegment> BlockSeg;

class ClangGenCtx : public IRPassBaseCtx<CodeSeg> {
public:
    ClangGenCtx(shared_ptr<Func> func, map<Sym, CodeSeg> m = {})
        : IRPassBaseCtx<CodeSeg>(func->tbl, m)
    {
    }
};

class ClangGen : public IRGenBase<CodeSeg> {
public:
    ClangGen(ClangGenCtx& ctx) : IRGenBase(ctx)
    {}

    static void Build(shared_ptr<Func>, llvm::Module&);

private:
    CodeSeg visit(Sym) final;
    CodeSeg visit(Call&) final;
    CodeSeg visit(IfElse&) final;
    CodeSeg visit(NoOp&) final;
    CodeSeg visit(Select&) final;
    CodeSeg visit(Const&) final;
    CodeSeg visit(Get&) final;
    CodeSeg visit(Cast&) final;
    CodeSeg visit(NaryExpr&) final;
    CodeSeg visit(Stmts&) final;
    CodeSeg visit(Alloc&) final;
    CodeSeg visit(Load&) final;
    CodeSeg visit(Store&) final;
    CodeSeg visit(AtomicOp&) final;
    CodeSeg visit(StructGEP&) final;
    CodeSeg visit(ThreadIdx&) final;
    CodeSeg visit(BlockIdx&) final;
    CodeSeg visit(BlockDim&) final;
    CodeSeg visit(GridDim&) final;
    CodeSeg visit(Loop&) final;
    CodeSeg visit(FetchDataPtr&) final;
    void visit(Func&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_CODEGEN_CLANGGEN_H_
