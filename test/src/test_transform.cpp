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

arrow::Status print_out_array(ArrowSchema* schema, ArrowArray* array)
{
    ARROW_ASSIGN_OR_RAISE(auto res, arrow::ImportRecordBatch(array, schema));
    return arrow::Status::OK();
}

void transform_test()
{
    auto loop = transform_loop();
    auto query_fn = compile_loop<void (*)(void*, void*, void*)>(loop);

    auto tbl = get_input_vector().ValueOrDie();
    auto* in_array = tbl->array;
    auto out_table = std::make_shared<ArrowTable>(
        "output", in_array->length,
        std::vector<std::string>{"id", "minutes_studied", "slept_enough"},
        std::vector<reffine::DataType>{types::INT64, types::INT64,
                                       types::BOOL});
    auto* out_schema = out_table->schema;
    auto* out_array = out_table->array;

    query_fn(&out_array, in_array, out_array);

    auto status = print_out_array(out_schema, out_array);
    if (!status.ok()) {
        ASSERT_EQ(1, 2);
    } else {
        ASSERT_EQ(1, 1);
    }
}
