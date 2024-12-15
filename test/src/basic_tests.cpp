#include "test_base.h"

TEST(BasicTests, ReduceTest) { aggregate_test(); }
TEST(BasicTests, TransformTest) { transform_test(); }

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
