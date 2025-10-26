#include "reffine/builder/reffiner.h"
#include "reffine/vinstr/vinstr.h"
#include "test_base.h"
#include "test_utils.h"

using namespace reffine;
using namespace reffine::reffiner;

shared_ptr<Func> join_op(ArrowTable2* a, ArrowTable2* b, ArrowTable2* c)
{
    auto avec_sym = _sym("avec", a->get_data_type());
    auto bvec_sym = _sym("bvec", b->get_data_type());
    auto cvec_sym = _sym("cvec", c->get_data_type());
    auto t_sym = _sym("t", _i64_t);

    auto aelem = avec_sym[{t_sym}][0];
    auto belem = bvec_sym[{t_sym}][0];
    auto celem = cvec_sym[{t_sym}][0];
    auto aelem_sym = _sym("a", aelem);
    auto belem_sym = _sym("b", belem);
    auto celem_sym = _sym("c", celem);

    auto op = _op(
        vector<Sym>{t_sym},
        (_in(t_sym, avec_sym) & _in(t_sym, bvec_sym) & _in(t_sym, cvec_sym)),
        vector<Expr>{_add(_add(aelem_sym, belem_sym), celem_sym)});
    auto op_sym = _sym("op", op);

    auto fn = _func("join", op_sym, vector<Sym>{avec_sym, bvec_sym, cvec_sym});
    fn->tbl[aelem_sym] = aelem;
    fn->tbl[belem_sym] = belem;
    fn->tbl[celem_sym] = celem;
    fn->tbl[op_sym] = op;

    return fn;
}

void join_op_test(bool vectorize)
{
    int64_t alb = 0, aub = 10, blb = 5, bub = 15, clb = 7, cub = 12;
    ArrowTable2* a_table;
    ArrowTable2* b_table;
    ArrowTable2* c_table;

    auto data_fn = gen_fake_table();
    ASSERT(alb <= aub && blb <= bub && clb <= cub);
    data_fn(&a_table, alb, aub);
    data_fn(&b_table, blb, bub);
    data_fn(&c_table, clb, cub);

    a_table->build_index();
    b_table->build_index();
    c_table->build_index();

    auto jop = join_op(a_table, b_table, c_table);
    auto join_fn =
        compile_op<void (*)(void*, void*, void*, void*)>(jop, vectorize);

    ArrowTable2* join_table;
    join_fn(&join_table, a_table, b_table, c_table);

    auto lb = std::max({alb, blb, clb});
    auto ub = std::min({aub, bub, cub});
    ASSERT_EQ(get_vector_len(join_table), ub - lb);

    auto* a_col0 = (int64_t*)get_vector_data_buf(a_table, 0);
    auto* a_col1 = (int64_t*)get_vector_data_buf(a_table, 1);
    auto* b_col0 = (int64_t*)get_vector_data_buf(b_table, 0);
    auto* b_col1 = (int64_t*)get_vector_data_buf(b_table, 1);
    auto* c_col0 = (int64_t*)get_vector_data_buf(c_table, 0);
    auto* c_col1 = (int64_t*)get_vector_data_buf(c_table, 1);
    auto* join_col0 = (int64_t*)get_vector_data_buf(join_table, 0);
    auto* join_col1 = (int64_t*)get_vector_data_buf(join_table, 1);

    int64_t a_i = vector_locate(a_table, lb);
    int64_t b_i = vector_locate(b_table, lb);
    int64_t c_i = vector_locate(c_table, lb);
    for (size_t i = 0; i < get_vector_len(join_table); i++) {
        ASSERT_EQ(a_col0[a_i], join_col0[i]);
        ASSERT_EQ(b_col0[b_i], join_col0[i]);
        ASSERT_EQ(c_col0[c_i], join_col0[i]);
        ASSERT_EQ(a_col1[a_i] + b_col1[b_i] + c_col1[c_i], join_col1[i]);
        a_i++;
        b_i++;
        c_i++;
    }
}
