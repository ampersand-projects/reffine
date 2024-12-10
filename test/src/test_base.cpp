#include "test_base.h"

#include <gtest/gtest.h>

#include "reffine/engine/engine.h"

using namespace reffine;

void foo_test()
{
    auto out = 1 + 1;
    auto true_out = 2;
    ASSERT_EQ(out, true_out);
}
