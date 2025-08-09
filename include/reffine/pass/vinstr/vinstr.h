#ifndef INCLUDE_REFFINE_PASS_VINSTR_H_
#define INCLUDE_REFFINE_PASS_VINSTR_H_

#include <arrow/c/abi.h>

#define REFFINE_VINSTR_ATTR __attribute__((always_inline))

extern "C" {

/**
 * Internal
 */
REFFINE_VINSTR_ATTR
int64_t get_vector_len(ArrowArray*);

REFFINE_VINSTR_ATTR
int64_t set_vector_len(ArrowArray*, int64_t);

REFFINE_VINSTR_ATTR
bool is_valid(ArrowArray*, int64_t);

REFFINE_VINSTR_ATTR
void set_valid(ArrowArray*, int64_t);

REFFINE_VINSTR_ATTR
void set_invalid(ArrowArray*, int64_t);

REFFINE_VINSTR_ATTR
bool get_vector_null_bit(ArrowArray*, int64_t, uint32_t);

REFFINE_VINSTR_ATTR
bool set_vector_null_bit(ArrowArray*, int64_t, bool, uint32_t);

REFFINE_VINSTR_ATTR
void* get_vector_data_buf(ArrowArray*, uint32_t);

REFFINE_VINSTR_ATTR
int64_t vector_lookup(ArrowArray*, int64_t);

REFFINE_VINSTR_ATTR
int64_t vector_locate(ArrowArray*, int64_t);

REFFINE_VINSTR_ATTR
int64_t* get_elem_ptr(int64_t*, int64_t);


/**
 * External
 */
REFFINE_VINSTR_ATTR
ArrowArray* make_vector();

}  // extern "C"

#endif  // INCLUDE_REFFINE_PASS_VINSTR_H_
