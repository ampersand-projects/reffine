#include "reffine/vinstr/vinstr.h"

int64_t get_vector_len(ArrowTable* tbl)
{
    return tbl->array->length;
}

int64_t set_vector_len(ArrowTable* tbl, int64_t len)
{
    auto* arr = tbl->array;
    arr->length = len;
    for (int i = 0; i < arr->n_children; i++) {
        arr->children[i]->length = len;
    }
    return arr->length;
}

static bool is_valid(ArrowArray* arr, int64_t idx)
{
    auto bitmap = (char*)arr->buffers[0];
    return (arr->null_count == 0) || (bitmap[idx / 8] & (1 << (idx % 8)));
}

static void set_valid(ArrowArray* arr, int64_t idx)
{
    auto bitmap = (char*)arr->buffers[0];
    bitmap[idx / 8] |= (1 << (idx % 8));
}

static void set_invalid(ArrowArray* arr, int64_t idx)
{
    auto bitmap = (char*)arr->buffers[0];
    bitmap[idx / 8] &= ~(1 << (idx % 8));
    arr->null_count++;
}

bool get_vector_null_bit(ArrowTable* tbl, int64_t idx, uint32_t col)
{
    auto* arr = tbl->array;
    return is_valid(arr, idx) && is_valid(arr->children[col], idx);
}

bool set_vector_null_bit(ArrowTable* tbl, int64_t idx, bool validity,
                         uint32_t col)
{
    auto* arr = tbl->array;

    // only setting the bitmap of children as arrow::ImportRecordBatch
    // disallows struct-level positive null count
    if (validity) {
        set_valid(arr->children[col], idx);
    } else {
        set_invalid(arr->children[col], idx);
    }

    return validity;
}

void* get_vector_data_buf(ArrowTable* tbl, uint32_t col)
{
    auto* arr = tbl->array;
    return (void*)arr->children[col]->buffers[1];
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
