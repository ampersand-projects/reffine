#include "reffine/pass/z3solver.h"
#include "reffine/builder/reffiner.h"
#include "test_base.h"
#include "test_utils.h"

using namespace reffine;
using namespace reffine::reffiner;

void run_lower_bound_check(int lb)
{
    auto t = _sym("t", types::INT64);
    auto pred = _gte(t, _i64(lb));
    auto bnd = _sym("bnd", t);
    auto prop = _forall(t, _iff(_gte(t, bnd), pred));

    Z3Solver solver;
    auto res = solver.check(prop);
    ASSERT_EQ(res, z3::sat);
    ASSERT_EQ(solver.get(bnd).as_int64(), lb);
}

void z3solver_test()
{
    run_lower_bound_check(10);
    run_lower_bound_check(133);
    run_lower_bound_check(55);
    run_lower_bound_check(0);
    run_lower_bound_check(-543);
}
