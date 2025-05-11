#ifndef TEST_INCLUDE_TEST_BASE_H_
#define TEST_INCLUDE_TEST_BASE_H_

#include <gtest/gtest.h>

#include "reffine/arrow/defs.h"
#include "reffine/builder/reffiner.h"

void aggregate_loop_test();
void aggregate_op_test();
void transform_test();
void z3solver_test();
void cuda_test();

#endif  // TEST_INCLUDE_TEST_BASE_H_
