#include "reffine/pass/reffinepass.h"

#include "reffine/pass/z3solver.h"

using namespace reffine;

Expr get_init_val(Sym idx, Expr pred)
{
    auto p = make_shared<SymNode>(idx->name + "_p", idx);
    auto forall = make_shared<ForAll>(
        idx,
        make_shared<And>(
            make_shared<Implies>(make_shared<GreaterThanEqual>(idx, p), pred),
            make_shared<Implies>(make_shared<LessThan>(idx, p),
                                 make_shared<Not>(pred))));

    Z3Solver solver;
    if (solver.check(forall) == z3::sat) {
        auto p_val = solver.get(p).as_int64();
        return make_shared<Const>(idx->type.btype, p_val);
    }

    return nullptr;
}

Expr get_exit_val(Sym idx, Expr pred)
{
    auto p = make_shared<SymNode>(idx->name + "_p", idx);
    auto forall = make_shared<ForAll>(
        idx, make_shared<And>(
                 make_shared<Implies>(make_shared<LessThanEqual>(idx, p), pred),
                 make_shared<Implies>(make_shared<GreaterThan>(idx, p),
                                      make_shared<Not>(pred))));

    Z3Solver solver;
    if (solver.check(forall) == z3::sat) {
        auto p_val = solver.get(p).as_int64();
        return make_shared<Const>(idx->type.btype, p_val);
    }

    return nullptr;
}

IterSpace ReffinePass::visit(NaryExpr& expr)
{
    IterSpace ispace;

    ispace.lower_bound = make_shared<Const>(BaseType::INT64, 0);
    ispace.upper_bound = make_shared<Const>(BaseType::INT64, 9);
    ispace.body_cond = nullptr;
    ispace.idx_to_iter = [](Expr idx) {
        return make_shared<Cast>(types::INT64, idx);
    };
    ispace.iter_to_idx = [](Expr iter) {
        return make_shared<Cast>(types::IDX, iter);
    };
    ispace.idx_incr = [](Expr idx) {
        return make_shared<Add>(idx, make_shared<Const>(BaseType::IDX, 1));
    };

    return ispace;
}

IterSpace ReffinePass::Build(Op& op)
{
    ReffinePassCtx ctx;
    ReffinePass rpass(ctx);

    op.pred->Accept(rpass);

    return rpass.val();
}
