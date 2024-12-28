#ifndef INCLUDE_REFFINE_PASS_REFFINEPASS_H_
#define INCLUDE_REFFINE_PASS_REFFINEPASS_H_

#include "reffine/pass/base/irgen.h"

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

using ReffinePassCtx = ValGenCtx<IterSpace>;

class ReffinePass : public ValGen<IterSpace> {
public:
    ReffinePass(ReffinePassCtx& ctx, Op& op) : ValGen<IterSpace>(ctx), _op(op)
    {
    }

    static IterSpace Build(Op&);

private:
    IterSpace visit(NaryExpr&) final;
    IterSpace visit(Sym) final;
    IterSpace visit(In&) final;

    Op& op() { return _op; }

    Op& _op;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_REFFINEPASS_H_
