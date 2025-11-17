#ifndef TEST_INCLUDE_TEST_BASE_H_
#define TEST_INCLUDE_TEST_BASE_H_

#include <gtest/gtest.h>

#include "reffine/arrow/table.h"
#include "reffine/builder/reffiner.h"

void aggregate_loop_test();
void aggregate_op_test(bool = false);
void transform_loop_test();
void transform_op_test(bool = false);
void nested_op_test(bool = false);
void join_op_test(bool = false);
void multidim_op_test(bool = false);
void z3solver_test();

#endif  // TEST_INCLUDE_TEST_BASE_H_
