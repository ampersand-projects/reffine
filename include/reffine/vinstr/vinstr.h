#ifndef INCLUDE_REFFINE_VINSTR_H_
#define INCLUDE_REFFINE_VINSTR_H_

#include "reffine/arrow/abi.h"

#ifndef REFFINE_VINSTR_ATTR
#define REFFINE_VINSTR_ATTR inline __attribute__((always_inline))
#endif

extern "C" {

/**
 * Internal
 */
REFFINE_VINSTR_ATTR
int64_t read_runend_buf(void* buf, int64_t idx)
{
    if (idx < 0) {
        return 0;
    } else {
        return (int64_t)((int32_t*)buf)[idx];
    }
}

REFFINE_VINSTR_ATTR
ArrowArray* get_vector_array(ArrowTable* tbl) { return tbl->array; }

REFFINE_VINSTR_ATTR
ArrowArray* get_array_child(ArrowArray* arr, uint32_t col)
{
    return arr->children[col];
}

REFFINE_VINSTR_ATTR
void* get_array_buf(ArrowArray* arr, uint32_t col)
{
    return (void*)arr->buffers[col];
}

REFFINE_VINSTR_ATTR
int64_t get_array_len(ArrowArray* arr) { return arr->length; }

REFFINE_VINSTR_ATTR
int64_t get_vector_len(ArrowTable* tbl) { return tbl->array->length; }

REFFINE_VINSTR_ATTR
int64_t set_vector_len(ArrowTable* tbl, int64_t len)
{
    auto* arr = tbl->array;
    arr->length = len;
    for (int i = 0; i < arr->n_children; i++) {
        arr->children[i]->length = len;
    }
    return arr->length;
}

REFFINE_VINSTR_ATTR
uint16_t* get_vector_bit_buf(ArrowTable* tbl, uint32_t col)
{
    return (uint16_t*)get_array_buf(get_array_child(get_vector_array(tbl), col),
                                    0);
}

REFFINE_VINSTR_ATTR
void* get_vector_data_buf(ArrowTable* tbl, uint32_t col)
{
    return (void*)get_array_buf(get_array_child(get_vector_array(tbl), col), 1);
}

REFFINE_VINSTR_ATTR
bool get_null_bit(uint16_t* bitmap, int64_t idx)
{
    return !bitmap || (bitmap[idx >> 4] & (1u << (idx & 15)));
}

REFFINE_VINSTR_ATTR
void set_null_bit(uint16_t* bitmap, int64_t idx, bool validity)
{
    if (bitmap) {
        uint16_t mask = 1u << (idx & 15);
        uint16_t* byte = &bitmap[idx >> 4];
        *byte = (*byte & ~mask) | (-validity & mask);
    }
}

REFFINE_VINSTR_ATTR
bool get_vector_null_bit(ArrowTable* tbl, int64_t idx, uint32_t col)
{
    auto* bitmap = get_vector_bit_buf(tbl, col);
    return get_null_bit(bitmap, idx);
}

REFFINE_VINSTR_ATTR
void set_vector_null_bit(ArrowTable* tbl, int64_t idx, bool validity,
                         uint32_t col)
{
    // only setting the bitmap of children as arrow::ImportRecordBatch
    // disallows struct-level positive null count
    auto* bitmap = get_vector_bit_buf(tbl, col);
    set_null_bit(bitmap, idx, validity);
}

REFFINE_VINSTR_ATTR
int64_t vector_lookup(ArrowTable* tbl, int64_t idx)
{
    auto buf = (int64_t*)get_vector_data_buf(tbl, 0);
    return buf[idx];
}

REFFINE_VINSTR_ATTR
int64_t* get_elem_ptr(int64_t* arr, int64_t idx) { return arr + idx; }

REFFINE_VINSTR_ATTR
void finalize_vector(ArrowTable* tbl, bool* bytemap, int64_t len,
                     int64_t null_count)
{
    auto* arr = tbl->array;

    for (int64_t col = 0; col < arr->n_children; col++) {
        if (null_count > 0) {
            int64_t i = 0;
            while (i < len) {
                set_vector_null_bit(tbl, i, bytemap[i], col);
                i++;
            }
        } else {
            arr->children[col]->buffers[0] = nullptr;
        }

        arr->children[col]->null_count = null_count;
        arr->children[col]->length = len;
    }

    arr->length = len;
}

REFFINE_VINSTR_ATTR
int64_t vector_locate(ArrowTable* tbl, int64_t val)
{
    auto it = tbl->index->find(val);
    if (it != tbl->index->end()) {
        return it->second;
    } else {
        return -1;
    }
}

/**
 * External
 */
ArrowTable* make_vector(int64_t, uint32_t);

ArrowTable* build_vector_index(ArrowTable*);

}  // extern "C"

#endif  // INCLUDE_REFFINE_VINSTR_H_
