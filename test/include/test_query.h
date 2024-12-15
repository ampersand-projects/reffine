#ifndef TEST_INCLUDE_TEST_QUERY_H_
#define TEST_INCLUDE_TEST_QUERY_H_

#include "reffine/ir/node.h"
#include "reffine/ir/stmt.h"
#include "reffine/ir/expr.h"
#include "reffine/ir/loop.h"
#include "reffine/ir/op.h"

shared_ptr<reffine::Func> vector_loop();
shared_ptr<reffine::Func> transform_loop();

#endif  // TEST_INCLUDE_TEST_QUERY_H_
