#ifndef INCLUDE_REFFINE_PASS_VINSTR_H_
#define INCLUDE_REFFINE_PASS_VINSTR_H_

#include <arrow/c/abi.h>

#define REFFINE_VINSTR_ATTR __attribute__((always_inline))

namespace reffine {
extern "C" {

REFFINE_VINSTR_ATTR int64_t get_vector_len(ArrowArray*);
REFFINE_VINSTR_ATTR int64_t read_val(ArrowArray*, int);
REFFINE_VINSTR_ATTR int64_t transform_val(ArrowArray*, ArrowArray*, int);

}  // extern "C"
}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_VINSTR_H_
