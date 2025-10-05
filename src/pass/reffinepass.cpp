#include "reffine/pass/reffinepass.h"

#include "reffine/builder/reffiner.h"
#include "reffine/pass/z3solver.h"

using namespace reffine;
using namespace reffine::reffiner;

ISpace Reffine::extract_bound(Sym iter, Expr pred)
{
    Z3Solver solver;

    auto bnd = _sym(iter->name + "_bnd", iter);
    auto lb_prop = _forall(iter, _iff(_gte(iter, bnd), pred));
    auto ub_prop = _forall(iter, _iff(_lte(iter, bnd), pred));

    auto [lb_check, lb_val] = Z3Solver::Solve(lb_prop, bnd);
    auto [ub_check, ub_val] = Z3Solver::Solve(ub_prop, bnd);

    if (lb_check == z3::sat) {
        auto lb_uniq_prop = _and(_not(_eq(bnd, lb_val)), lb_prop);
        auto [uniq_check, _] = Z3Solver::Solve(lb_uniq_prop);
        if (uniq_check == z3::unsat) {
            return eval(iter) >= lb_val;
        }
    } else if (ub_check == z3::sat) {
        auto ub_uniq_prop = _and(_not(_eq(bnd, ub_val)), ub_prop);
        auto [uniq_check, _] = Z3Solver::Solve(ub_uniq_prop);
        if (uniq_check == z3::unsat) {
            return eval(iter) <= ub_val;
        }
    }

    throw runtime_error("Unidentified bound condition");
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
            return extract_bound(op().iters[0], this->tmp_expr(e));
        default:
            throw runtime_error("Operator not supported by Reffine");
    }
}

ISpace Reffine::visit(Sym sym)
{
    if (sym == op().iters[0]) {
        // return universal space for operator iterator
        return make_shared<UniversalSpace>(sym->type);
    } else if (sym->type.is_vector()) {
        return make_shared<VecSpace>(sym);
    } else {
        return eval(this->ctx().in_sym_tbl.at(sym));
    }
}

ISpace Reffine::visit(Element& elem)
{
    ASSERT(elem.iters.size() == 1);
    ASSERT(elem.iters[0] == op().iters[0]);

    // Assuming Element is only visited through NotNull
    // Therefore, always returning vector space
    return eval(elem.vec);
}

ISpace Reffine::visit(NotNull& not_null)
{
    // Assuming NotNull always traslates to vector space
    return eval(not_null.elem);
}

ISpace Reffine::Build(Op& op, const SymTable& tbl)
{
    // Only support single iterator for now
    ASSERT(op.iters.size() == 1);

    Reffine rpass(make_unique<ReffineCtx>(tbl), op);

    return rpass.eval(op.pred);
}
