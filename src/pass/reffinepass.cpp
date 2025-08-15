#include "reffine/pass/reffinepass.h"

#include "reffine/builder/reffiner.h"
#include "reffine/pass/z3solver.h"

using namespace reffine;
using namespace reffine::reffiner;

static Expr get_lower_bound(Sym iter, Expr pred)
{
    auto p = _sym(iter->name + "_p", iter);
    auto forall = _forall(iter, _and(_implies(_gte(iter, p), pred),
                                     _implies(_lt(iter, p), _not(pred))));

    Z3Solver solver;
    if (solver.check(forall) == z3::sat) {
        auto p_val = solver.get(p).as_int64();
        return _const(iter->type, p_val);
    }

    return nullptr;
}

static Expr get_upper_bound(Sym iter, Expr pred)
{
    auto p = _sym(iter->name + "_p", iter);
    auto forall = _forall(iter, _and(_implies(_lte(iter, p), pred),
                                     _implies(_gt(iter, p), _not(pred))));

    Z3Solver solver;
    if (solver.check(forall) == z3::sat) {
        auto p_val = solver.get(p).as_int64();
        return _const(iter->type, p_val);
    }

    return nullptr;
}

static bool contains_vecspace(ISpace ispace)
{
    if (dynamic_cast<VecSpace*>(ispace.get())) { return true; }

    if (auto jspace = dynamic_cast<JointSpace*>(ispace.get())) {
        auto left = jspace->left;
        auto right = jspace->right;
        if (contains_vecspace(left) || contains_vecspace(right)) {
            return true;
        }
    }

    if (auto bspace = dynamic_cast<BoundSpace*>(ispace.get())) {
        auto child = bspace->ispace;
        if (contains_vecspace(child)) { return true; }
    }

    return false;
}

static ISpace apply_intersection(ISpace ispace, ISpace bspace)
{
    if (auto lbspace = dynamic_cast<LBoundSpace*>(bspace.get())) {
        return ispace >= lbspace->lower_bound();
    }
    if (auto ubspace = dynamic_cast<UBoundSpace*>(bspace.get())) {
        return ispace <= ubspace->upper_bound();
    }
    return ispace;
}

static ISpace apply_union(ISpace ispace, ISpace bspace)
{
    if (auto lbspace = dynamic_cast<LBoundSpace*>(bspace.get())) {
        auto lb1 = ispace->lower_bound();
        auto lb2 = lbspace->lower_bound();
        return ispace >= (lb1 ? _min(lb2, lb1) : lb2);
    }
    if (auto ubspace = dynamic_cast<UBoundSpace*>(bspace.get())) {
        auto ub1 = ispace->upper_bound();
        auto ub2 = ubspace->upper_bound();
        return ispace <= (ub1 ? _min(ub2, ub1) : ub2);
        ;
    }
    return ispace;
}

static ISpace intersect_ispaces(ISpace left, ISpace right)
{
    if (contains_vecspace(left) && contains_vecspace(right)) {
        return left & right;
    }
    if (contains_vecspace(left)) { return apply_intersection(left, right); }
    if (contains_vecspace(right)) { return apply_intersection(right, left); }
    return apply_intersection(left, right);
}

static ISpace union_ispaces(ISpace left, ISpace right)
{
    if (contains_vecspace(left) && contains_vecspace(right)) {
        return left | right;
    }
    if (contains_vecspace(left)) { return apply_union(left, right); }
    if (contains_vecspace(right)) { return apply_union(right, left); }
    return apply_union(left, right);
}

ISpace Reffine::visit(NaryExpr& e)
{
    switch (e.op) {
        case MathOp::AND: {
            return intersect_ispaces(eval(e.arg(0)), eval(e.arg(1)));
        }
        case MathOp::OR: {
            return union_ispaces(eval(e.arg(0)), eval(e.arg(1)));
        }
        case MathOp::LT:
        case MathOp::LTE:
        case MathOp::GT:
        case MathOp::GTE: {
            auto iter = op().iters[0];
            auto pred = this->tmp_expr(e);

            if (auto lb = get_lower_bound(iter, pred)) {
                return eval(iter) >= lb;
            } else if (auto ub = get_upper_bound(iter, pred)) {
                return eval(iter) <= ub;
            } else {
                throw runtime_error("Unabled to identify the bounds");
            }
        }
        default:
            throw runtime_error("Operator not supported by Reffine");
    }
}

ISpace Reffine::visit(Sym sym)
{
    if (sym == op().iters[0]) {
        // return universal space for operator iterator
        return make_shared<IterSpace>(sym->type);
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

    ReffineCtx ctx(tbl);
    Reffine rpass(ctx, op);

    return rpass.eval(op.pred);
}
