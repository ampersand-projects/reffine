#ifndef INCLUDE_REFFINE_PASS_IRCLONE_H_
#define INCLUDE_REFFINE_PASS_IRCLONE_H_

#include "reffine/pass/base/irgen.h"

namespace reffine {

class IRCloneCtx : public IRGenCtx {
public:
    IRCloneCtx(shared_ptr<Func> old_func, shared_ptr<Func> new_func)
        : IRGenCtx(old_func->tbl, new_func->tbl), new_func(new_func)
    {
    }

private:
    shared_ptr<Func> new_func;

    friend class IRClone;
};

class IRClone : public IRGen {
public:
    explicit IRClone(IRCloneCtx& ctx) : IRGen(ctx), _irclonectx(ctx) {}

    static shared_ptr<Func> Build(shared_ptr<Func> func);

protected:
    Expr visit(Sym) override;
    Expr visit(Select&) override;
    Expr visit(IfElse&) override;
    Expr visit(Const&) override;
    Expr visit(Cast&) override;
    Expr visit(Get&) override;
    Expr visit(New&) override;
    Expr visit(NaryExpr&) override;
    Expr visit(Op&) override;
    Expr visit(Element&) override;
    Expr visit(NotNull&) override;
    Expr visit(Reduce&) override;
    Expr visit(Call&) override;
    Expr visit(Stmts&) override;
    Expr visit(Alloc&) override;
    Expr visit(Load&) override;
    Expr visit(Store&) override;
    Expr visit(AtomicOp&) override;
    Expr visit(StructGEP&) override;
    Expr visit(ThreadIdx&) override;
    Expr visit(BlockIdx&) override;
    Expr visit(BlockDim&) override;
    Expr visit(GridDim&) override;
    Expr visit(Loop&) override;
    Expr visit(FetchDataPtr&) override;
    Expr visit(NoOp&) override;
    void visit(Func&) override;

private:
    shared_ptr<Op> visit_op(Op&);

    IRCloneCtx& _irclonectx;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_IRCLONE_H_
