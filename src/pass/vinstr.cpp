#include <cstdio>

#include "reffine/pass/vinstr.h"

namespace reffine {
extern "C" {

int64_t get_vector_len(ArrowArray* arr) { return arr->length; }

int64_t read_val(ArrowArray* arr, int idx)
{
    auto* id_col = (int64_t*) arr->children[0]->buffers[1];
    auto* hours_studied_col = (int64_t*) arr->children[1]->buffers[1];
    auto i = id_col[idx];
    auto k = hours_studied_col[idx];
    //printf("Val is %ld: %ld\n", i, k);
    return k;
}

int64_t transform_val(ArrowArray* in_array, ArrowArray* out_array, int i) {
    auto* in_id_array = in_array->children[0];
    auto* in_id_col = (int64_t*) in_id_array->buffers[1];
    auto* in_hours_array = in_array->children[1];
    auto* in_hours_col = (int64_t*) in_hours_array->buffers[1];

    auto* out_id_array = out_array->children[0];
    auto* out_id_bitmap = (char*) out_id_array->buffers[0];
    auto* out_id_col = (int64_t*) out_id_array->buffers[1];
    auto* out_min_array = out_array->children[1];
    auto* out_min_bitmap = (char*) out_min_array->buffers[0];
    auto* out_min_col = (int64_t*) out_min_array->buffers[1];

    out_array->length++;
    out_id_array->length++;
    out_min_array->length++;
    out_id_bitmap[i/8] |= (1 << (i%8));
    out_id_col[i] = in_id_col[i];
    out_min_bitmap[i/8] |= (1 << (i%8));
    out_min_col[i] = in_hours_col[i]*60;

    return i;
}

}  // extern "C"
}  // namespace reffine
