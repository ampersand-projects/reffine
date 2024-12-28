#include "reffine/pass/reffinepass.h"
#include "reffine/pass/z3solver.h"

using namespace reffine;

Expr get_lower_bound(Sym iter, Expr pred)
{
    auto p = make_shared<SymNode>(iter->name + "_p", iter);
    auto forall = make_shared<ForAll>(
        iter,
        make_shared<And>(
            make_shared<Implies>(make_shared<GreaterThanEqual>(iter, p), pred),
            make_shared<Implies>(make_shared<LessThan>(iter, p),
                                 make_shared<Not>(pred))));

    Z3Solver solver;
    if (solver.check(forall) == z3::sat) {
        auto p_val = solver.get(p).as_int64();
        return make_shared<Const>(iter->type.btype, p_val);
    }

    return nullptr;
}

Expr get_upper_bound(Sym iter, Expr pred)
{
    auto p = make_shared<SymNode>(iter->name + "_p", iter);
    auto forall = make_shared<ForAll>(
        iter, make_shared<And>(
                 make_shared<Implies>(make_shared<LessThanEqual>(iter, p), pred),
                 make_shared<Implies>(make_shared<GreaterThan>(iter, p),
                                      make_shared<Not>(pred))));

    Z3Solver solver;
    if (solver.check(forall) == z3::sat) {
        auto p_val = solver.get(p).as_int64();
        return make_shared<Const>(iter->type.btype, p_val);
    }

    return nullptr;
}

IterSpace apply_bounds(IterSpace ispace, Expr pred)
{
    ASSERT(ispace.space->type.is_val());

    if (auto lb = get_lower_bound(ispace.space, pred)) {
        ispace.lower_bound = make_shared<Max>(ispace.lower_bound, lb);
    } else if (auto ub = get_upper_bound(ispace.space, pred)) {
        ispace.upper_bound = make_shared<Min>(ispace.upper_bound, ub);
    } else {
        throw runtime_error("Unabled to identify the bounds");
    }

    return ispace;
}

// Temporary implementation. Need to revisit
IterSpace intersect(IterSpace a, IterSpace b)
{
    ASSERT(!(a.space->type.is_vector() && b.space->type.is_vector()));

    IterSpace* left;
    IterSpace* right;

    if (a.space->type.is_vector()) {
        left = &a;
        right = &b;
    } else {
        left = &b;
        right = &a;
    }

    IterSpace ispace;

    ispace.space = left->space;
    ispace.lower_bound = make_shared<Max>(left->lower_bound, right->lower_bound);
    ispace.upper_bound = make_shared<Min>(left->upper_bound, right->upper_bound);
    ispace.body_cond = make_shared<And>(left->body_cond, right->body_cond);
    ispace.iter_to_idx = left->iter_to_idx;
    ispace.idx_to_iter = left->idx_to_iter;
    ispace.idx_incr = left->idx_incr;

    return ispace;
}

IterSpace Reffine::visit(NaryExpr& e)
{
    switch (e.op) {
        case MathOp::AND:
            return intersect(eval(e.arg(0)), eval(e.arg(1)));
        case MathOp::LT:
        case MathOp::LTE:
        case MathOp::GT:
        case MathOp::GTE:
            return apply_bounds(eval(e.arg(0)), this->tmp_expr(e));
        default:
            throw runtime_error("Operator not supported by Reffine");
    }
}

IterSpace Reffine::visit(Sym sym)
{
    IterSpace ispace;

    if (sym == op().iters[0]) {
        ispace.space = sym;
        ispace.lower_bound = make_shared<Const>(sym->type.btype, -INF);
        ispace.upper_bound = make_shared<Const>(sym->type.btype, INF);
        ispace.body_cond = make_shared<Const>(BaseType::BOOL, 1);
        ispace.idx_to_iter = [sym](Expr idx) {
            return make_shared<Cast>(sym->type, idx);
        };
        ispace.iter_to_idx = [](Expr iter) {
            return make_shared<Cast>(types::IDX, iter);
        };
        ispace.idx_incr = [](Expr idx) {
            return make_shared<Add>(idx, make_shared<Const>(BaseType::IDX, 1));
        };
    } else if (sym->type.is_vector()) {
        ispace.space = sym;
        ispace.lower_bound = make_shared<Call>("vector_lookup", sym->type.dtypes[0], vector<Expr>{sym, make_shared<Const>(BaseType::IDX, 0)});

        auto len = make_shared<Call>("get_vector_len", types::IDX, vector<Expr>{sym});
        auto last_idx = make_shared<Sub>(len, make_shared<Const>(BaseType::IDX, 1));
        ispace.upper_bound = make_shared<Call>("vector_lookup", sym->type.dtypes[0], vector<Expr>{sym, last_idx});
        ispace.body_cond = make_shared<Const>(BaseType::BOOL, 1);
        ispace.idx_to_iter = [sym](Expr idx) {
            return make_shared<Call>("vector_lookup", sym->type.dtypes[0], vector<Expr>{sym, idx});
        };
        ispace.iter_to_idx = [sym](Expr iter) {
            return make_shared<Call>("vector_locate", types::IDX, vector<Expr>{sym, iter});
        };
        ispace.idx_incr = [](Expr idx) {
            return make_shared<Add>(idx, make_shared<Const>(BaseType::IDX, 1));
        };
    } else {
        throw runtime_error("Something went wrong");
    }

    return ispace;
}

IterSpace Reffine::visit(Element& elem)
{
    auto iter_ispace = eval(elem.iters[0]);
    auto vec_isapce = eval(elem.vec);

    return intersect(vec_isapce, iter_ispace);
}

IterSpace Reffine::Build(Op& op)
{
    ReffineCtx ctx;
    Reffine rpass(ctx, op);

    return rpass.eval(op.pred);
}
