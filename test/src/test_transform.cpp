#include "reffine/builder/reffiner.h"
#include "reffine/vinstr/vinstr.h"
#include "test_base.h"
#include "test_utils.h"

using namespace reffine;
using namespace reffine::reffiner;

shared_ptr<Func> transform_op(shared_ptr<ArrowTable2> tbl, long lb, long n)
{
    auto t_sym = _sym("t", _i64_t);
    auto vec_in_sym = _sym("vec_in", tbl->get_data_type(1));
    auto elem = vec_in_sym[{t_sym}];
    auto elem_sym = _sym("elem", elem);
    auto out = _add(elem_sym[0], _i64(n));
    auto out_sym = _sym("out", out);
    auto op =
        _op(vector<Sym>{t_sym}, _in(t_sym, vec_in_sym) & _gt(t_sym, _i64(lb)),
            vector<Expr>{out_sym});
    auto op_sym = _sym("op", op);

    auto foo_fn = _func("foo", op_sym, vector<Sym>{vec_in_sym});
    foo_fn->tbl[elem_sym] = elem;
    foo_fn->tbl[out_sym] = out;
    foo_fn->tbl[op_sym] = op;

    return foo_fn;
}

void transform_op_test(bool vectorize)
{
    auto lb = 5;
    auto n = 10;

    auto in_tbl = get_input_vector().ValueOrDie();
    auto op = transform_op(in_tbl, lb, n);
    auto query_fn =
        compile_op<void (*)(ArrowTable**, ArrowTable*)>(op, vectorize);

    ArrowTable* out_tbl;
    query_fn(&out_tbl, in_tbl.get());

    ASSERT_EQ(get_vector_len(in_tbl.get()) - lb, get_vector_len(out_tbl));

    auto* in_col0 = (int64_t*)get_vector_data_buf(in_tbl.get(), 0);
    auto* in_col1 = (int64_t*)get_vector_data_buf(in_tbl.get(), 1);
    auto* out_col0 = (int64_t*)get_vector_data_buf(out_tbl, 0);
    auto* out_col1 = (int64_t*)get_vector_data_buf(out_tbl, 1);
    for (size_t i = 5; i < get_vector_len(in_tbl.get()); i++) {
        ASSERT_EQ(in_col0[i], out_col0[i - lb]);
        ASSERT_EQ((in_col1[i] + n), out_col1[i - lb]);
    }
}
