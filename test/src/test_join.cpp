#include "reffine/builder/reffiner.h"
#include "reffine/vinstr/vinstr.h"
#include "test_base.h"
#include "test_utils.h"

using namespace reffine;
using namespace reffine::reffiner;

shared_ptr<Func> join_op(ArrowTable2* left, ArrowTable2* right)
{
    auto lvec_sym = _sym("left", left->get_data_type(1));
    auto rvec_sym = _sym("right", right->get_data_type(1));
    auto t_sym = _sym("t", _i64_t);

    auto lelem = lvec_sym[{t_sym}][0];
    auto relem = rvec_sym[{t_sym}][0];
    auto lelem_sym = _sym("l", lelem);
    auto relem_sym = _sym("r", relem);

    auto op =
        _op(vector<Sym>{t_sym}, (_in(t_sym, lvec_sym) & _in(t_sym, rvec_sym)),
            vector<Expr>{_add(lelem_sym, relem_sym)});
    auto op_sym = _sym("op", op);

    auto fn = _func("join", op_sym, vector<Sym>{lvec_sym, rvec_sym});
    fn->tbl[lelem_sym] = lelem;
    fn->tbl[relem_sym] = relem;
    fn->tbl[op_sym] = op;

    return fn;
}

void join_op_test(bool vectorize)
{
    int64_t llb = 0, lub = 10, rlb = 5, rub = 15;
    ArrowTable2* left_table;
    ArrowTable2* right_table;

    auto data_fn = gen_fake_table();
    data_fn(&left_table, llb, lub);
    data_fn(&right_table, rlb, rub);

    auto jop = join_op(left_table, right_table);
    auto join_fn = compile_op<void (*)(void*, void*, void*)>(jop, vectorize);

    ArrowTable2* join_table;
    join_fn(&join_table, left_table, right_table);

    ASSERT_EQ(get_vector_len(join_table), lub - rlb);

    auto* left_col0 = (int64_t*)get_vector_data_buf(left_table, 0);
    auto* left_col1 = (int64_t*)get_vector_data_buf(left_table, 1);
    auto* right_col0 = (int64_t*)get_vector_data_buf(right_table, 0);
    auto* right_col1 = (int64_t*)get_vector_data_buf(right_table, 1);
    auto* join_col0 = (int64_t*)get_vector_data_buf(join_table, 0);
    auto* join_col1 = (int64_t*)get_vector_data_buf(join_table, 1);

    int64_t t = std::min(lub, rlb);
    int64_t left_i = vector_locate(left_table, t);
    int64_t right_i = vector_locate(right_table, t);
    for (size_t i = 0; i < get_vector_len(join_table); i++) {
        ASSERT_EQ(left_col0[left_i], join_col0[i]);
        ASSERT_EQ(right_col0[right_i], join_col0[i]);
        ASSERT_EQ(left_col1[left_i] + right_col1[right_i], join_col1[i]);
        left_i++;
        right_i++;
    }
}
