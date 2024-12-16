#include "reffine/pass/z3solver.h"
#include "test_base.h"
#include "test_utils.h"
#include "z3++.h"

using namespace reffine;

Expr demorgan_expr()
{
    auto x = make_shared<SymNode>("x", types::BOOL);
    auto y = make_shared<SymNode>("y", types::BOOL);

    auto not_x = make_shared<Not>(x);
    auto not_y = make_shared<Not>(y);
    auto not_x_or_not_y = make_shared<Or>(not_x, not_y);

    auto x_and_y = make_shared<And>(x, y);
    auto not_x_and_y = make_shared<Not>(x_and_y);

    auto conjecture =
        make_shared<Not>(make_shared<Equals>(not_x_and_y, not_x_or_not_y));

    return conjecture;
}

void z3solver_test()
{
    Z3Solver z3s;
    auto conjecture = demorgan_expr();
    auto s = z3s.solve(conjecture);
    auto check = s.check();

    ASSERT_EQ(check, z3::unsat);
}
