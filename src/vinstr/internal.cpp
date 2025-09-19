#include "reffine/vinstr/vinstr.h"

int64_t get_vector_len(ArrowTable* tbl) { return tbl->array->length; }

int64_t set_vector_len(ArrowTable* tbl, int64_t len)
{
    auto* arr = tbl->array;
    arr->length = len;
    for (int i = 0; i < arr->n_children; i++) {
        arr->children[i]->length = len;
    }
    return arr->length;
}

uint16_t* get_vector_bit_buf(ArrowTable* tbl, uint32_t col)
{
    return (uint16_t*) tbl->array->children[col]->buffers[0];
}

void* get_vector_data_buf(ArrowTable* tbl, uint32_t col)
{
    return (void*) tbl->array->children[col]->buffers[1];
}

bool get_null_bit(uint16_t* bitmap, int64_t idx)
{
    return !bitmap || (bitmap[idx >> 4] & (1u << (idx & 15)));
}

void set_null_bit(uint16_t* bitmap, int64_t idx, bool validity)
{
    if (bitmap) {
        uint16_t mask = 1u << (idx & 15);
        uint16_t* byte = &bitmap[idx >> 4];
        *byte = (*byte & ~mask) | (-validity & mask);
    }
}

bool get_vector_null_bit(ArrowTable* tbl, int64_t idx, uint32_t col)
{
    auto* bitmap = get_vector_bit_buf(tbl, col);
    return get_null_bit(bitmap, idx);
}

void set_vector_null_bit(ArrowTable* tbl, int64_t idx, bool validity,
                         uint32_t col)
{
    // only setting the bitmap of children as arrow::ImportRecordBatch
    // disallows struct-level positive null count
    auto* bitmap = get_vector_bit_buf(tbl, col);
    set_null_bit(bitmap, idx, validity);
}

int64_t vector_lookup(ArrowTable* tbl, int64_t idx)
{
    auto buf = (int64_t*)get_vector_data_buf(tbl, 0);
    return buf[idx];
}

int64_t vector_locate(ArrowTable* tbl, int64_t t)
{
    for (int i = 0; i < get_vector_len(tbl); i++) {
        if (vector_lookup(tbl, i) == t) { return i; }
    }
    return -1;
}

int64_t* get_elem_ptr(int64_t* arr, int64_t idx) { return arr + idx; }
