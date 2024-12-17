#ifndef INCLUDE_REFFINE_PASS_Z3SOLVER_H_
#define INCLUDE_REFFINE_PASS_Z3SOLVER_H_

#include "reffine/pass/visitor.h"
#include "z3++.h"

namespace reffine {

class Z3Solver : public Visitor {
public:
    z3::expr solve(Expr, Expr);
    z3::check_result check(Expr);
    z3::expr get(Expr);

private:
    void Visit(SymNode&) final;
    void Visit(Const&) final;
    void Visit(NaryExpr&) final;

    z3::expr eval(Expr expr);
    z3::expr& val() { return _val; }
    z3::solver& s() { return _s; }
    z3::context& ctx() { return _ctx; }
    void assign(z3::expr e) { swap(e, val()); }

    z3::context _ctx;
    z3::expr _val = _ctx.bool_val(true);
    z3::solver _s = _ctx;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_Z3SOLVER_H_
