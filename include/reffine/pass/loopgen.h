#ifndef INCLUDE_REFFINE_PASS_LOOPGEN_H_
#define INCLUDE_REFFINE_PASS_LOOPGEN_H_

#include "reffine/pass/irclone.h"

namespace reffine {

using LoopGenCtx = IRCloneCtx;

class LoopGen : public IRClone {
public:
    explicit LoopGen(LoopGenCtx& ctx) :
        IRClone(ctx)
    {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

private:
    tuple<Sym, Expr> visit(Sym, Expr) final;
    Expr visit(Call&) final;
    Expr visit(Select&) final;
    Expr visit(Const&) final;
    Expr visit(Cast&) final;
    Expr visit(Get&) final;
    Expr visit(NaryExpr&) final;
    Expr visit(Op&) final;
    Expr visit(Element&) final;
    Expr visit(Reduce&) final;
    void visit(Func&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_LOOPGEN_H_
