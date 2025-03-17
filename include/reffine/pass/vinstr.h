// #ifndef INCLUDE_REFFINE_PASS_VINSTR_H_
// #define INCLUDE_REFFINE_PASS_VINSTR_H_

// #include "reffine/arrow/abi.h"

// #define REFFINE_VINSTR_ATTR __attribute__((always_inline))

// namespace reffine {
// extern "C" {

// REFFINE_VINSTR_ATTR
// int64_t get_vector_len(ArrowArray* arr) { return arr->length; }

// REFFINE_VINSTR_ATTR
// int64_t set_vector_len(ArrowArray* arr, int64_t len)
// {
//     arr->length = len;
//     for (int i = 0; i < arr->n_children; i++) {
//         arr->children[i]->length = len;
//     }
//     return arr->length;
// }

// REFFINE_VINSTR_ATTR
// static bool is_valid(ArrowArray* arr, int64_t idx)
// {
//     auto bitmap = (char*)arr->buffers[0];
//     return (arr->null_count == 0) || (bitmap[idx / 8] & (1 << (idx % 8)));
// }

// REFFINE_VINSTR_ATTR
// static void set_valid(ArrowArray* arr, int64_t idx)
// {
//     auto bitmap = (char*)arr->buffers[0];
//     bitmap[idx / 8] |= (1 << (idx % 8));
// }

// REFFINE_VINSTR_ATTR
// static void set_invalid(ArrowArray* arr, int64_t idx)
// {
//     auto bitmap = (char*)arr->buffers[0];
//     bitmap[idx / 8] &= ~(1 << (idx % 8));
//     arr->null_count++;
// }

// REFFINE_VINSTR_ATTR
// bool get_vector_null_bit(ArrowArray* arr, int64_t idx, uint32_t col)
// {
//     return is_valid(arr, idx) && is_valid(arr->children[col], idx);
// }

// REFFINE_VINSTR_ATTR
// bool set_vector_null_bit(ArrowArray* arr, int64_t idx, bool validity,
//                          uint32_t col)
// {
//     // only setting the bitmap of children as arrow::ImportRecordBatch
//     // disallows struct-level positive null count
//     if (validity) {
//         set_valid(arr->children[col], idx);
//     } else {
//         set_invalid(arr->children[col], idx);
//     }

//     return validity;
// }

// REFFINE_VINSTR_ATTR
// void* get_vector_data_buf(ArrowArray* arr, uint32_t col)
// {
//     return (void*)arr->children[col]->buffers[1];
// }

// REFFINE_VINSTR_ATTR
// int64_t vector_lookup(ArrowArray* arr, int64_t idx)
// {
//     auto buf = (int64_t*)get_vector_data_buf(arr, 0);
//     return buf[idx];
// }

// REFFINE_VINSTR_ATTR
// int64_t vector_locate(ArrowArray* arr, int64_t t)
// {
//     for (int i = 0; i < get_vector_len(arr); i++) {
//         if (vector_lookup(arr, i) == t) { return i; }
//     }
//     return -1;
// }

// REFFINE_VINSTR_ATTR
// int64_t* get_elem_ptr(int64_t* arr, int64_t idx)
// {
//     return arr + idx;
// }

// }  // extern "C"
// }  // namespace reffine

// #endif  // INCLUDE_REFFINE_PASS_VINSTR_H_
