#include "test_base.h"
#include "test_utils.h"
#include "test_query.h"

#include <gtest/gtest.h>

#include "reffine/pass/printer.h"
#include "reffine/pass/canonpass.h"
#include "reffine/pass/loopgen.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/engine/engine.h"

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
