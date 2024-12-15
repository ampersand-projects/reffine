#ifndef INCLUDE_REFFINE_PASS_IRCLONE_H_
#define INCLUDE_REFFINE_PASS_IRCLONE_H_

#include "reffine/pass/irgen.h"

namespace reffine {

class IRCloneCtx : public IRGenCtx<Sym, Expr> {
public:
    IRCloneCtx(shared_ptr<Func> old_func, shared_ptr<Func> new_func)
        : IRGenCtx(old_func->tbl, new_func->tbl), new_func(new_func)
    {
    }

private:
    shared_ptr<Func> new_func;

    friend class IRClone;
};

class IRClone : public IRGen<Sym, Expr> {
public:
    explicit IRClone(IRCloneCtx& ctx) : IRGen(ctx), _ctx(ctx) {}

protected:
    tuple<Sym, Expr> visit(Sym, Expr) override;
    Expr visit(Call&) override;
    Expr visit(Select&) override;
    Expr visit(Const&) override;
    Expr visit(Cast&) override;
    Expr visit(Get&) override;
    Expr visit(New&) override;
    Expr visit(NaryExpr&) override;
    Expr visit(Op&) override;
    Expr visit(Element&) override;
    Expr visit(Reduce&) override;
    void visit(Func&) override;

private:
    shared_ptr<Op> visit_op(Op&);

    IRCloneCtx& _ctx;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_IRCLONE_H_
