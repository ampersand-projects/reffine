#include "reffine/builder/reffiner.h"
#include "test_base.h"
#include "test_utils.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

shared_ptr<Func> vector_loop(shared_ptr<ArrowTable2> tbl)
{
    auto vec_sym = make_shared<SymNode>("vec", tbl->get_data_type());

    auto len =
        make_shared<Call>("get_vector_len", types::IDX, vector<Expr>{vec_sym});
    auto len_sym = make_shared<SymNode>("len", len);

    auto idx_alloc = make_shared<Alloc>(types::IDX);
    auto idx_addr = make_shared<SymNode>("idx_addr", idx_alloc);
    auto idx = make_shared<Load>(idx_addr);
    auto sum_alloc = make_shared<Alloc>(types::INT64);
    auto sum_addr = make_shared<SymNode>("sum_addr", sum_alloc);
    auto sum = make_shared<Load>(sum_addr);

    auto val_ptr = make_shared<FetchDataPtr>(vec_sym, 1);
    auto val = make_shared<Load>(val_ptr, idx);

    auto loop = _loop(make_shared<Load>(sum_addr));
    auto loop_sym = make_shared<SymNode>("loop", loop);
    loop->init = make_shared<Stmts>(vector<Expr>{
        make_shared<Store>(idx_addr, make_shared<Const>(types::IDX, 0)),
        make_shared<Store>(sum_addr, make_shared<Const>(types::INT64, 0)),
    });
    loop->exit_cond = make_shared<GreaterThanEqual>(idx, len_sym);
    loop->body = make_shared<Stmts>(vector<Expr>{
        make_shared<Store>(sum_addr, make_shared<Add>(sum, val)),
        make_shared<Store>(
            idx_addr, make_shared<Add>(idx, make_shared<Const>(types::IDX, 1))),
    });

    auto foo_fn = make_shared<Func>("foo", loop_sym, vector<Sym>{vec_sym});
    foo_fn->tbl[len_sym] = len;
    foo_fn->tbl[idx_addr] = idx_alloc;
    foo_fn->tbl[sum_addr] = sum_alloc;
    foo_fn->tbl[loop_sym] = loop;

    return foo_fn;
}

void aggregate_loop_test()
{
    auto tbl = get_input_vector().ValueOrDie();
    auto loop = vector_loop(tbl);
    long output = 0;
    auto query_fn = compile_loop<void (*)(long*, void*)>(loop);

    query_fn(&output, tbl.get());

    ASSERT_EQ(output, 131977);
}

shared_ptr<Func> vector_op(shared_ptr<ArrowTable2> tbl)
{
    auto t_sym = _sym("t", _i64_t);
    auto vec_in_sym = _sym("vec_in", tbl->get_data_type());
    Op op(
        {t_sym},
        _in(t_sym, vec_in_sym) & _lte(t_sym, _i64(48)) & _gte(t_sym, _i64(10)),
        {
            vec_in_sym[{t_sym}][2],
            _new(vector<Expr>{
                vec_in_sym[{t_sym}][1],
                vec_in_sym[{t_sym}][2],
                vec_in_sym[{t_sym}][0],
                vec_in_sym[{t_sym}][3],
            }),
            vec_in_sym[{t_sym}][3],
        });

    auto sum = _red(
        op,
        []() {
            return _new(vector<Expr>{
                _new(vector<Expr>{_i64(0), _i64(0)}),
                _new(vector<Expr>{_i64(0), _i64(0)}),
            });
        },
        [](Expr s, Expr v) {
            auto v0 = _get(_get(v, 2), 0);
            auto v1 = _get(_get(v, 2), 1);
            auto v2 = _get(_get(v, 2), 2);
            auto v3 = _get(_get(v, 2), 3);
            auto s0 = _get(_get(s, 0), 0);
            auto s1 = _get(_get(s, 0), 1);
            auto s2 = _get(_get(s, 1), 0);
            auto s3 = _get(_get(s, 1), 1);
            return _new(vector<Expr>{
                _new(vector<Expr>{_add(s0, v0), _add(s1, v1)}),
                _new(vector<Expr>{_add(s2, v2), _add(s3, v3)}),
            });
        });
    auto sum_sym = _sym("sum", sum);
    auto res = _get(_get(sum_sym, 1), 0);
    auto res_sym = _sym("res", res);

    auto foo_fn = _func("foo", res_sym, vector<Sym>{vec_in_sym});
    foo_fn->tbl[sum_sym] = sum;
    foo_fn->tbl[res_sym] = res;

    return foo_fn;
}

void aggregate_op_test(bool vectorize)
{
    auto tbl = get_input_vector().ValueOrDie();
    auto op = vector_op(tbl);
    long output = 0;
    auto query_fn = compile_op<void (*)(long*, void*)>(op, vectorize);

    query_fn(&output, tbl.get());

    ASSERT_EQ(output, 696);
}
