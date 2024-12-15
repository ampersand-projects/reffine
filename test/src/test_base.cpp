#include "test_base.h"
#include "test_utils.h"
#include "test_query.h"

#include <gtest/gtest.h>

#include "reffine/arrow/defs.h"

using namespace reffine;

void foo_test()
{
    auto out = 1 + 1;
    auto true_out = 2;
    ASSERT_EQ(out, true_out);
}

void aggregate_test()
{
    auto loop = vector_loop();
    auto query_fn = compile_loop<long (*)(void*)>(loop);

    auto tbl = get_input_vector();
    auto res = query_fn(&tbl->array);

    ASSERT_EQ(res, 131977);
}

void transform_test()
{
    auto loop = transform_loop();
    auto query_fn = compile_loop<void* (*)(void*, void*)>(loop);

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

    auto res = query_fn(&in_array, &out_array);

    ASSERT_EQ(1, 1);
    
}
