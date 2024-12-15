#include "test_base.h"
#include "test_utils.h"

using namespace reffine;

shared_ptr<Func> vector_loop()
{
    auto vec_sym = make_shared<SymNode>("vec", types::VECTOR<1>(vector<DataType>{
        types::INT64, types::INT64, types::INT64, types::INT64, types::INT64, types::INT8, types::INT64 }));

    auto len = make_shared<Call>("get_vector_len", types::IDX, vector<Expr>{vec_sym});
    auto len_sym = make_shared<SymNode>("len", len);

    auto idx_alloc = make_shared<Alloc>(types::IDX);
    auto idx_addr = make_shared<SymNode>("idx_addr", idx_alloc);
    auto idx = make_shared<Load>(idx_addr);
    auto sum_alloc = make_shared<Alloc>(types::INT64);
    auto sum_addr = make_shared<SymNode>("sum_addr", sum_alloc);
    auto sum = make_shared<Load>(sum_addr);

    auto val_ptr = make_shared<FetchDataPtr>(vec_sym, idx, 1);
    auto val = make_shared<Load>(val_ptr);

    auto loop = make_shared<Loop>(make_shared<Load>(sum_addr));
    auto loop_sym = make_shared<SymNode>("loop", loop);
    loop->init = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(idx_addr, make_shared<Const>(BaseType::IDX, 0)),
        make_shared<Store>(sum_addr, make_shared<Const>(BaseType::INT64, 0)),
    });
    loop->exit_cond = make_shared<GreaterThanEqual>(idx, len_sym);
    loop->body = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(sum_addr, make_shared<Add>(sum, val)),
        make_shared<Store>(idx_addr, make_shared<Add>(idx, make_shared<Const>(BaseType::IDX, 1))),
    });

    auto foo_fn = make_shared<Func>("foo", loop_sym, vector<Sym>{vec_sym});
    foo_fn->tbl[len_sym] = len;
    foo_fn->tbl[idx_addr] = idx_alloc;
    foo_fn->tbl[sum_addr] = sum_alloc;
    foo_fn->tbl[loop_sym] = loop;

    return foo_fn;
}

void aggregate_test()
{
    auto loop = vector_loop();
    auto query_fn = compile_loop<long (*)(void*)>(loop);

    auto tbl = get_input_vector();
    auto res = query_fn(&tbl->array);

    ASSERT_EQ(res, 131977);
}
