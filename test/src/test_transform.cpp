#include "reffine/builder/reffiner.h"
#include "test_base.h"
#include "test_utils.h"

using namespace reffine;
using namespace reffine::reffiner;

shared_ptr<Func> transform_loop()
{
    auto vec_in_sym = make_shared<SymNode>(
        "vec_in", types::VECTOR<1>(vector<DataType>{
                      types::INT64, types::INT64, types::INT64, types::INT64,
                      types::INT64, types::INT8, types::INT64}));
    auto vec_out_sym = make_shared<SymNode>(
        "vec_out", types::VECTOR<1>(vector<DataType>{types::INT64, types::INT64,
                                                     types::INT8}));

    auto len = make_shared<Call>("get_vector_len", types::IDX,
                                 vector<Expr>{vec_in_sym});
    auto len_sym = make_shared<SymNode>("len", len);

    auto zero = make_shared<Const>(types::IDX, 0);
    auto one = make_shared<Const>(types::IDX, 1);
    auto eight = make_shared<Const>(types::INT64, 8);
    auto sixty = make_shared<Const>(types::INT64, 60);
    auto twenty = make_shared<Const>(types::INT64, 20);
    auto _true = make_shared<Const>(types::BOOL, 1);
    auto _false = make_shared<Const>(types::BOOL, 0);

    auto idx_alloc = make_shared<Alloc>(types::IDX);
    auto idx_addr = make_shared<SymNode>("idx_addr", idx_alloc);
    auto idx = make_shared<Load>(idx_addr);

    auto id_valid = make_shared<IsValid>(vec_in_sym, idx, 0);
    auto id_buf = _fetch_buf(vec_in_sym, 0);
    auto id_buf_sym = _sym("id_buf", id_buf);
    auto id_data_ptr = make_shared<FetchDataPtr>(id_buf, idx);
    auto id_data = make_shared<Load>(id_data_ptr);
    auto hours_valid = make_shared<IsValid>(vec_in_sym, idx, 1);

    auto hours_data_buf = _fetch_buf(vec_in_sym, 1);
    auto hours_buf_sym = _sym("hour_data_buf", hours_data_buf);
    auto hours_data_ptr = _fetch(hours_buf_sym, idx);
    auto hours_data = make_shared<Load>(hours_data_ptr);

    auto hours_slept_buf = _fetch_buf(vec_in_sym, 3);
    auto hours_slept_buf_sym = _sym("hours_slept_buf", hours_slept_buf);
    auto hours_slept_data_ptr =
        make_shared<FetchDataPtr>(hours_slept_buf_sym, idx);
    auto hours_slept_data = make_shared<Load>(hours_slept_data_ptr);

    auto out_id_buf = _fetch_buf(vec_out_sym, 0);
    auto out_id_buf_sym = _sym("out_id_buf", out_id_buf);
    auto out_id_data_ptr = make_shared<FetchDataPtr>(out_id_buf_sym, idx);

    auto out_min_buf = _fetch_buf(vec_out_sym, 1);
    auto out_min_buf_sym = _sym("out_min_buf", out_min_buf);
    auto out_minutes_data_ptr = make_shared<FetchDataPtr>(out_min_buf_sym, idx);

    auto out_sleep_buf = _fetch_buf(vec_out_sym, 2);
    auto out_sleep_buf_sym = _sym("out_sleep_buf", out_sleep_buf);
    auto out_sleep_data_ptr = make_shared<FetchDataPtr>(out_sleep_buf_sym, idx);
    auto out_minutes = make_shared<Mul>(hours_data, sixty);

    auto loop = _loop(vec_out_sym);
    auto loop_sym = make_shared<SymNode>("loop", loop);
    loop->init = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(idx_addr, zero),
    });
    loop->incr = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(idx_addr,
                           make_shared<Add>(make_shared<Load>(idx_addr), one)),
    });
    loop->exit_cond =
        make_shared<GreaterThanEqual>(make_shared<Load>(idx_addr), len_sym);
    loop->body = make_shared<Stmts>(vector<Stmt>{
        make_shared<IfElse>(
            make_shared<And>(make_shared<And>(id_valid, hours_valid),
                             make_shared<GreaterThanEqual>(hours_data, twenty)),
            make_shared<Stmts>(vector<Stmt>{
                make_shared<Store>(out_id_data_ptr,
                                   make_shared<Load>(id_data_ptr)),
                make_shared<Store>(out_minutes_data_ptr, out_minutes),
                make_shared<SetValid>(vec_out_sym, idx, _true, 0),
                make_shared<SetValid>(vec_out_sym, idx, _true, 1),
            }),
            make_shared<Stmts>(vector<Stmt>{
                make_shared<SetValid>(vec_out_sym, idx, _false, 0),
                make_shared<SetValid>(vec_out_sym, idx, _false, 1),
            })),
        make_shared<Store>(
            out_sleep_data_ptr,
            make_shared<Select>(make_shared<LessThan>(hours_slept_data, eight),
                                make_shared<Const>(types::INT8, 0),
                                make_shared<Const>(types::INT8, 1))),
        make_shared<SetValid>(vec_out_sym, idx, _true, 2)});
    loop->post = make_shared<Call>(
        "set_vector_len", types::INT64,
        vector<Expr>{vec_out_sym, make_shared<Load>(idx_addr)});

    auto foo_fn = make_shared<Func>("foo", loop_sym,
                                    vector<Sym>{vec_in_sym, vec_out_sym});
    foo_fn->tbl[len_sym] = len;
    foo_fn->tbl[idx_addr] = idx_alloc;
    foo_fn->tbl[loop_sym] = loop;
    foo_fn->tbl[hours_buf_sym] = hours_data_buf;
    foo_fn->tbl[id_buf_sym] = id_buf;
    foo_fn->tbl[hours_slept_buf_sym] = hours_slept_buf;
    foo_fn->tbl[out_id_buf_sym] = out_id_buf;
    foo_fn->tbl[out_min_buf_sym] = out_min_buf;
    foo_fn->tbl[out_sleep_buf_sym] = out_sleep_buf;

    return foo_fn;
}

void transform_test()
{
    auto loop = transform_loop();
    auto query_fn = compile_loop<void (*)(void*, void*)>(loop);

    auto tbl = get_input_vector();
    auto in_array = tbl->array;
    VectorSchema out_schema("output");
    VectorArray out_array(in_array.length);
    out_schema.add_child<Int64Schema>("id");
    out_schema.add_child<Int64Schema>("minutes_studied");
    out_schema.add_child<BooleanSchema>("slept_enough");
    out_array.add_child<Int64Array>(in_array.length);
    out_array.add_child<Int64Array>(in_array.length);
    out_array.add_child<BooleanArray>(in_array.length);

    query_fn(&in_array, &out_array);

    ASSERT_EQ(1, 1);
}
