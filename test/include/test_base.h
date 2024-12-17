#ifndef TEST_INCLUDE_TEST_BASE_H_
#define TEST_INCLUDE_TEST_BASE_H_

#include <gtest/gtest.h>

#include "reffine/arrow/defs.h"
#include "reffine/ir/loop.h"
#include "reffine/ir/op.h"
#include "reffine/ir/stmt.h"

void aggregate_loop_test();
void aggregate_op_test();
void transform_test();
void z3solver_test();

#endif  // TEST_INCLUDE_TEST_BASE_H_
