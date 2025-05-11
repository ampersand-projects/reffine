#include "test_base.h"
#include "test_utils.h"

using namespace reffine;

void run_kernel_check(int x)
{
    ASSERT_EQ(x, x);
}

void cuda_test()
{
    run_kernel_check(10);
}
