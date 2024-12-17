#include "reffine/pass/z3solver.h"
#include "test_base.h"
#include "test_utils.h"
#include "z3++.h"

using namespace reffine;

tuple<Expr, Expr> lower_bound_expr(int lb)
{
    auto t = make_shared<SymNode>("t", types::INT64);
    auto zero = make_shared<Const>(BaseType::INT64, lb);
    auto pred = make_shared<GreaterThanEqual>(t, zero);

    auto p = make_shared<SymNode>("p", t);
    auto forall = make_shared<ForAll>(t,
            make_shared<And>(
                make_shared<Implies>(make_shared<GreaterThanEqual>(t, p), pred),
                make_shared<Implies>(make_shared<LessThan>(t, p), make_shared<Not>(pred))
                )
            );
    return {forall, p};
}

void run_lower_bound_check(int lb)
{
    Z3Solver z3s;

    auto [conj, val] = lower_bound_expr(lb);
    auto s = z3s.solve(conj, val).as_int64();

    ASSERT_EQ(s, lb);
}

void z3solver_test()
{
    run_lower_bound_check(10);
    run_lower_bound_check(133);
    run_lower_bound_check(55);
    run_lower_bound_check(0);
    run_lower_bound_check(-543);
}
