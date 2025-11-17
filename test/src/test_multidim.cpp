#include "reffine/builder/reffiner.h"
#include "reffine/vinstr/vinstr.h"
#include "test_base.h"
#include "test_utils.h"

using namespace reffine;
using namespace reffine::reffiner;

shared_ptr<Func> multidim_op(shared_ptr<ArrowTable2> tbl)
{
    auto vec_sym = _sym("vec_in", tbl->get_data_type());
    auto t_sym = _sym("t_sym", _i64_t);

    auto red = _red(
        vec_sym[t_sym], []() { return _i64(0); },
        [](Expr s, Expr v) {
            auto v1 = _get(v, 1);
            return _add(s, v1);
        });
    auto red_sym = _sym("sum", red);
    auto op =
        _op(vector<Sym>{t_sym}, _in(t_sym, vec_sym), vector<Expr>{red_sym});
    auto op_sym = _sym("op", op);

    auto fn = _func("red", op_sym, vector<Sym>{vec_sym});
    fn->tbl[op_sym] = op;
    fn->tbl[red_sym] = red;

    return fn;
}

void multidim_op_test(bool vectorize)
{
    auto in_tbl = get_input_vector(RUNEND_ARROW_FILE, 2).ValueOrDie();
    auto op = multidim_op(in_tbl);
    auto query_fn =
        compile_op<void (*)(ArrowTable**, ArrowTable*)>(op, vectorize);

    ArrowTable* out_tbl;
    query_fn(&out_tbl, in_tbl.get());

    ASSERT_EQ(in_tbl->array->children[0]->children[1]->length,
              out_tbl->array->length);

    auto* out_col0 = (int64_t*)get_vector_data_buf(out_tbl, 0);
    auto* out_col1 = (int64_t*)get_vector_data_buf(out_tbl, 1);
    ASSERT_EQ(out_col0[0], 1);
    ASSERT_EQ(out_col0[1], 2);
    ASSERT_EQ(out_col0[2], 3);
    ASSERT_EQ(out_col0[3], 4);
    ASSERT_EQ(out_col1[0], 3);
    ASSERT_EQ(out_col1[1], 7);
    ASSERT_EQ(out_col1[2], 26);
    ASSERT_EQ(out_col1[3], 9);
}
