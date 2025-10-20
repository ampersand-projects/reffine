#ifndef TEST_INCLUDE_TEST_UTILS_H_
#define TEST_INCLUDE_TEST_UTILS_H_

#include <memory>
#include <string>

#include "arrow/api.h"
#include "arrow/c/bridge.h"
#include "arrow/csv/api.h"
#include "arrow/io/api.h"
#include "arrow/ipc/api.h"
#include "arrow/result.h"
#include "arrow/status.h"
#include "reffine/arrow/table.h"
#include "reffine/engine/engine.h"
#include "reffine/pass/canonpass.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/pass/loopgen.h"
#include "reffine/pass/reffinepass.h"
#include "reffine/pass/scalarpass.h"
#include "reffine/utils/utils.h"

arrow::Result<std::shared_ptr<reffine::ArrowTable2>> get_input_vector();

std::string print_arrow_table(ArrowTable*);

typedef void (*gen_table_ty)(void*, int64_t, int64_t);
gen_table_ty gen_fake_table();
#endif  // TEST_INCLUDE_TEST_UTILS_H_
