#include "reffine/builder/reffiner.h"
#include "reffine/vinstr/vinstr.h"
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
    auto id_data_ptr = make_shared<FetchDataPtr>(vec_in_sym, idx, 0);
    auto id_data = make_shared<Load>(id_data_ptr);
    auto hours_valid = make_shared<IsValid>(vec_in_sym, idx, 1);
    auto hours_data_ptr = make_shared<FetchDataPtr>(vec_in_sym, idx, 1);
    auto hours_data = make_shared<Load>(hours_data_ptr);
    auto hours_slept_data_ptr = make_shared<FetchDataPtr>(vec_in_sym, idx, 3);
    auto hours_slept_data = make_shared<Load>(hours_slept_data_ptr);

    auto out_id_data_ptr = make_shared<FetchDataPtr>(vec_out_sym, idx, 0);
    auto out_minutes_data_ptr = make_shared<FetchDataPtr>(vec_out_sym, idx, 1);
    auto out_sleep_data_ptr = make_shared<FetchDataPtr>(vec_out_sym, idx, 2);
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

    return foo_fn;
}

void transform_loop_test()
{
    auto loop = transform_loop();
    auto query_fn = compile_loop<void (*)(void*, void*, void*)>(loop);

    auto in_table = get_input_vector().ValueOrDie();
    auto out_table = std::make_shared<ArrowTable2>(
        "output", in_table->array->length,
        std::vector<std::string>{"id", "minutes_studied", "slept_enough"},
        std::vector<reffine::DataType>{types::INT64, types::INT64,
                                       types::BOOL});
    ArrowTable* out_table2;
    query_fn(&out_table2, in_table.get(), out_table.get());

    std::cout << print_arrow_table(out_table.get());
}

shared_ptr<Func> transform_op(shared_ptr<ArrowTable2> tbl, long lb, long n)
{
    auto t_sym = _sym("t", _i64_t);
    auto vec_in_sym = _sym("vec_in", tbl->get_data_type(1));
    auto elem = vec_in_sym[{t_sym}];
    auto elem_sym = _sym("elem", elem);
    auto out = _add(elem_sym[0], _i64(n));
    auto out_sym = _sym("out", out);
    auto op = _op(vector<Sym>{t_sym}, ~(elem_sym)&_gt(t_sym, _i64(lb)),
                  vector<Expr>{out_sym});
    auto op_sym = _sym("op", op);

    auto foo_fn = _func("foo", op_sym, vector<Sym>{vec_in_sym});
    foo_fn->tbl[elem_sym] = elem;
    foo_fn->tbl[out_sym] = out;
    foo_fn->tbl[op_sym] = op;

    return foo_fn;
}

void transform_op_test()
{
    auto lb = 5;
    auto n = 10;

    auto in_tbl = get_input_vector().ValueOrDie();
    auto op = transform_op(in_tbl, lb, n);
    auto query_fn = compile_op<void (*)(ArrowTable**, ArrowTable*)>(op);

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
