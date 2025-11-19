#include "reffine/pass/reffinepass.h"

#include "reffine/builder/reffiner.h"
#include "reffine/pass/z3solver.h"

using namespace reffine;
using namespace reffine::reffiner;

ISpace Reffine::extract_bound(NaryExpr& e)
{
    auto pred = this->tmp_expr(e);
    auto bias = _sym("b_" + this->iter()->name, this->iter());
    Expr bound = bias;

    vector<Expr> vars;
    vars.push_back(this->iter());
    map<Sym, Sym> var_weight_map;
    for (auto var : this->vars()) {
        auto weight = _sym("w_" + var->name, var);
        // Note: `bound` should always be the second argument of _add().
        // Otherwise, for some reason SMT solver fails to satisfy valid prop
        bound = _add(_mul(weight, var), bound);
        var_weight_map[var] = weight;
        vars.push_back(var);
    }

    auto lb_prop = _forall(vars, _eq(_gte(this->iter(), bound), pred));
    auto ub_prop = _forall(vars, _eq(_lte(this->iter(), bound), pred));

    Z3Solver lb_solver, ub_solver;
    auto lb_check = lb_solver.check(lb_prop);
    auto ub_check = ub_solver.check(ub_prop);

    if (lb_check || ub_check) {
        auto& solver = lb_check ? lb_solver : ub_solver;
        Expr bound_val = solver.get(bias);
        for (auto [v, w] : var_weight_map) {
            auto weight = solver.get(w);
            if (weight->val != 0) {
                bound_val = _add(bound_val, _mul(weight, v));
            }
        }
        auto iter_ispace = eval(this->iter());
        auto const_ispace = make_shared<ConstantSpace>(bound_val);
        if (lb_check) {
            return make_shared<LBoundSpace>(iter_ispace, const_ispace);
        } else {
            return make_shared<UBoundSpace>(iter_ispace, const_ispace);
        }
    } else {
        return make_shared<UniversalSpace>(this->iter());
    }
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
            return extract_bound(e);
        default:
            throw runtime_error("Operator not supported by Reffine");
    }
}

ISpace Reffine::visit(Sym sym)
{
    auto uni = make_shared<UniversalSpace>(this->iter());
    if (sym == this->iter()) {
        // return universal space for operator iterator
        return uni;
    } else if (this->ctx().in_sym_tbl.find(sym) !=
               this->ctx().in_sym_tbl.end()) {
        return make_shared<FilteredSpace>(uni, sym);
    } else {
        throw runtime_error("Unable to reffine symbol");
    }
}

ISpace Reffine::visit(Const& cnst)
{
    return make_shared<ConstantSpace>(_const(cnst.type, cnst.val));
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
        this->vars().insert(iters[0]);
        op.iters = vector<Sym>(iters.begin() + 1, iters.end());
        auto inner_ispace = eval(this->tmp_expr(op));
        op.iters = iters;

        return make_shared<NestedSpace>(ispace, inner_ispace);
    } else {
        return ispace;
    }
}
