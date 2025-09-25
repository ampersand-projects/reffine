#ifndef INCLUDE_REFFINE_VINSTR_H_
#define INCLUDE_REFFINE_VINSTR_H_

#include "reffine/arrow/abi.h"

#define REFFINE_VINSTR_ATTR __attribute__((always_inline))

extern "C" {

/**
 * Internal
 */
REFFINE_VINSTR_ATTR
int64_t get_vector_len(ArrowTable*);

REFFINE_VINSTR_ATTR
int64_t set_vector_len(ArrowTable*, int64_t);

REFFINE_VINSTR_ATTR
bool get_vector_null_bit(ArrowTable*, int64_t, uint32_t);

REFFINE_VINSTR_ATTR
void set_vector_null_bit(ArrowTable*, int64_t, bool, uint32_t);

REFFINE_VINSTR_ATTR
uint16_t* get_vector_bit_buf(ArrowTable*, uint32_t);

REFFINE_VINSTR_ATTR
void* get_vector_data_buf(ArrowTable*, uint32_t);

REFFINE_VINSTR_ATTR
bool get_null_bit(uint16_t*, int64_t);

REFFINE_VINSTR_ATTR
void set_null_bit(uint16_t*, int64_t, bool);

REFFINE_VINSTR_ATTR
int64_t vector_lookup(ArrowTable*, int64_t);

REFFINE_VINSTR_ATTR
int64_t vector_locate(ArrowTable*, int64_t);

REFFINE_VINSTR_ATTR
int64_t* get_elem_ptr(int64_t*, int64_t);

REFFINE_VINSTR_ATTR
void finalize_vector(ArrowTable*, bool*, int64_t);

/**
 * External
 */
ArrowTable* make_vector(int64_t, uint32_t);

}  // extern "C"

#endif  // INCLUDE_REFFINE_VINSTR_H_
