#include "test_base.h"

TEST(BasicTests, FooTest) { foo_test(); }
TEST(BasicTests, ReduceTest) { aggregate_test(); }

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
