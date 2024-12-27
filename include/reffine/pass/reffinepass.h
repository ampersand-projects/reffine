#ifndef INCLUDE_REFFINE_PASS_REFFINEPASS_H_
#define INCLUDE_REFFINE_PASS_REFFINEPASS_H_

#include "reffine/pass/base/irgen.h"

namespace reffine {

struct IterSpace {
    Sym iter;
    Sym space;
    Expr lower_bound;
    Expr upper_bound;
    Expr body_cond;
    std::function<Expr(Expr)> iter_to_idx;
    std::function<Expr(Expr)> idx_to_iter;
    std::function<Expr(Expr)> idx_incr;
};

class ReffineCtx : public IRPassBaseCtx<IterSpace> {
public:
    ReffineCtx(const SymTable& in_sym_tbl, map<Sym, IterSpace> m = {}) : IRPassBaseCtx<IterSpace>(in_sym_tbl, m) {}
};

class ReffinePass : public IRGenBase<IterSpace> {
public:
    ReffinePass(ReffineCtx& ctx) : IRGenBase<IterSpace>(ctx) {}

    static IterSpace Build(Op&);

private:
    IterSpace visit(NaryExpr&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_REFFINEPASS_H_
