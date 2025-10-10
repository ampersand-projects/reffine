#include "reffine/pass/reffinepass.h"

#include "reffine/builder/reffiner.h"
#include "reffine/pass/z3solver.h"

using namespace reffine;
using namespace reffine::reffiner;

ISpace Reffine::extract_bound(Sym iter, NaryExpr& expr)
{
    Z3Solver solver;

    auto bnd = _sym(iter->name + "_bnd", iter);
    auto pred = this->tmp_expr(expr);
    auto lb_prop = _forall(iter, _iff(_gte(iter, bnd), pred));
    auto ub_prop = _forall(iter, _iff(_lte(iter, bnd), pred));

    auto [lb_check, lb_val] = Z3Solver::Solve(lb_prop, bnd);
    auto [ub_check, ub_val] = Z3Solver::Solve(ub_prop, bnd);

    if (lb_check == z3::sat) {
        auto lb_uniq_prop = (bnd != lb_val) & lb_prop;
        auto [uniq_check, _] = Z3Solver::Solve(lb_uniq_prop);
        if (uniq_check == z3::unsat) {
            return eval(iter) >= lb_val;
        } else if (expr.arg(0) == iter) {
            return eval(iter) >= expr.arg(1);
        }
    } else if (ub_check == z3::sat) {
        auto ub_uniq_prop = (bnd != ub_val) & ub_prop;
        auto [uniq_check, _] = Z3Solver::Solve(ub_uniq_prop);
        if (uniq_check == z3::unsat) {
            return eval(iter) <= ub_val;
        } else if (expr.arg(0) == iter) {
            return eval(iter) <= expr.arg(1);
        }
    }

    // Expression is not a bound on the given iter
    return eval(iter);
}

ISpace Reffine::visit(NaryExpr& e)
{
    switch (e.op) {
        case MathOp::AND:
            return eval(e.arg(0)) & eval(e.arg(1));
        case MathOp::OR:
            return eval(e.arg(0)) | eval(e.arg(1));
        case MathOp::LT:
        case MathOp::LTE:
        case MathOp::GT:
        case MathOp::GTE:
            return extract_bound(iter(), e);
        default:
            throw runtime_error("Operator not supported by Reffine");
    }
}

ISpace Reffine::visit(Sym sym)
{
    if (sym == iter()) {
        // return universal space for operator iterator
        return make_shared<UniversalSpace>(iter());
    } else {
        return eval(this->ctx().in_sym_tbl.at(sym));
    }
}

ISpace Reffine::visit(In& in)
{
    ASSERT(in.iter == iter());
    return make_shared<VecSpace>(iter(), in.vec);
}

ISpace Reffine::visit(Op& op)
{
    this->iter() = op.iters[0];
    auto ispace = eval(op.pred);

    if (op.iters.size() > 1) {
        auto iters = op.iters;
        this->sym_set().insert(iters[0]);
        op.iters = vector<Sym>(iters.begin() + 1, iters.end());
        auto inner_ispace = eval(this->tmp_expr(op));
        op.iters = iters;

        return make_shared<NestedSpace>(ispace, inner_ispace);
    } else {
        return ispace;
    }
}
