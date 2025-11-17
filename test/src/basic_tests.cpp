#include "test_base.h"

TEST(BasicTests, ReduceLoopTest) { aggregate_loop_test(); }
TEST(BasicTests, ReduceOpTest) { aggregate_op_test(); }
TEST(BasicTests, TransformOpTest) { transform_op_test(); }
TEST(BasicTests, NestedOpTest) { nested_op_test(); }
TEST(BasicTests, JoinOpTest) { join_op_test(); }
TEST(BasicTests, MultiDimOpTest) { multidim_op_test(); }
TEST(BasicTests, Z3SolverTest) { z3solver_test(); }

TEST(VectorizeTests, ReduceOpTest) { aggregate_op_test(true); }
TEST(VectorizeTests, TransformOpTest) { transform_op_test(true); }
TEST(VectorizeTests, NestedOpTest) { nested_op_test(true); }
TEST(VectorizeTests, JoinOpTest) { join_op_test(true); }
TEST(VectorizeTests, MultiDimOpTest) { multidim_op_test(true); }

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
