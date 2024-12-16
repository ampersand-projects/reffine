#ifndef INCLUDE_REFFINE_PASS_Z3SOLVER_H_
#define INCLUDE_REFFINE_PASS_Z3SOLVER_H_

#include "reffine/pass/visitor.h"
#include "z3++.h"

namespace reffine {

class Z3Solver : public Visitor {
public:
    z3::solver solve(Expr);

private:
    void Visit(SymNode&) final;
    void Visit(Const&) final;
    void Visit(NaryExpr&) final;

    z3::expr eval(Expr expr);
    z3::context& ctx() { return _ctx; }
    z3::expr& val() { return _val; }
    void assign(z3::expr e) { swap(e, val()); }

    z3::context _ctx;
    z3::expr _val = _ctx.bool_val(true);
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_Z3SOLVER_H_
