#include "test_base.h"

TEST(BasicTests, ReduceLoopTest) { aggregate_loop_test(); }
TEST(BasicTests, ReduceOpTest) { aggregate_op_test(); }
TEST(BasicTests, TransformTest) { transform_test(); }
TEST(BasicTests, Z3SolverTest) { z3solver_test(); }
#ifdef ENABLE_CUDA
TEST(BasicTests, CudaTest) { cuda_test(); }
#endif

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
