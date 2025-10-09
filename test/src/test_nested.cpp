#include "reffine/builder/reffiner.h"
#include "reffine/vinstr/vinstr.h"
#include "test_base.h"
#include "test_utils.h"

using namespace reffine;
using namespace reffine::reffiner;

shared_ptr<Func> nested_op(int a_ub, int b_ub)
{
    auto a_sym = _sym("a", _i64_t);
    auto b_sym = _sym("b", _i64_t);

    auto op = _op(vector<Sym>{a_sym, b_sym},
                  (_gte(a_sym, _i64(0)) & _lt(a_sym, _i64(a_ub)) &
                   _gte(b_sym, _i64(0)) & _lt(b_sym, _i64(b_ub))),
                  vector<Expr>{a_sym + b_sym});

    auto op_sym = _sym("op", op);

    auto foo_fn = _func("foo", op_sym, vector<Sym>{});
    foo_fn->tbl[op_sym] = op;

    return foo_fn;
}

void nested_op_test(bool vectorize)
{
    int a_ub = 10;
    int b_ub = 10;

    auto op = nested_op(a_ub, b_ub);
    auto query_fn = compile_op<void (*)(ArrowTable**)>(op, vectorize);

    ArrowTable* out_tbl;
    query_fn(&out_tbl);

    auto* out_data_a = (int64_t*)get_vector_data_buf(out_tbl, 0);
    auto* out_data_b = (int64_t*)get_vector_data_buf(out_tbl, 1);
    auto* out_data_a_b = (int64_t*)get_vector_data_buf(out_tbl, 2);

    int idx = 0;
    for (int i = 0; i < get_vector_len(out_tbl); i++) {
        auto cond = get_vector_null_bit(out_tbl, i, 0);
        if (cond) {
            auto a = idx / a_ub;
            auto b = idx % b_ub;
            ASSERT_EQ(out_data_a[i], a);
            ASSERT_EQ(out_data_b[i], b);
            ASSERT_EQ(out_data_a_b[i], a + b);
            idx++;
        }
    }
}
