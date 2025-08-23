#include "test_base.h"

TEST(BasicTests, ReduceLoopTest) { aggregate_loop_test(); }
TEST(BasicTests, ReduceOpTest) { aggregate_op_test(); }
TEST(BasicTests, TransformLoopTest) { transform_loop_test(); }
TEST(BasicTests, TransformOpTest) { transform_op_test(); }
TEST(BasicTests, Z3SolverTest) { z3solver_test(); }

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
