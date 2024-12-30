#ifndef INCLUDE_REFFINE_PASS_REFFINEPASS_H_
#define INCLUDE_REFFINE_PASS_REFFINEPASS_H_

#include "reffine/pass/base/irgen.h"
#include "reffine/pass/irclone.h"

namespace reffine {

struct IterSpace {
    Sym space;
    Expr lower_bound;
    Expr upper_bound;
    Expr body_cond;
    std::function<Expr(Expr)> iter_to_idx;
    std::function<Expr(Expr)> idx_to_iter;
    std::function<Expr(Expr)> idx_incr;
};

using ReffineCtx = ValGenCtx<IterSpace>;

class Reffine : public ValGen<IterSpace> {
public:
    Reffine(ReffineCtx& ctx, Op& op) : ValGen<IterSpace>(ctx), _op(op) {}

    static IterSpace Build(Op&);

private:
    IterSpace visit(NaryExpr&) final;
    IterSpace visit(Sym) final;
    IterSpace visit(Element&) final;
    IterSpace visit(NotNull&) final;

    Op& op() { return _op; }

    Op& _op;
};

using OpToLoopCtx = IRCloneCtx;

class OpToLoop : public IRClone {
public:
    explicit OpToLoop(OpToLoopCtx& ctx) : IRClone(ctx) {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

private:
    shared_ptr<Op> reffine(Op&);
    Expr visit(Op&) final;
    Expr visit(Reduce&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_REFFINEPASS_H_
