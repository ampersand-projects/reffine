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
    printf("Val is %ld: %ld\n", i, k);
    return k;
}

}  // extern "C"
}  // namespace reffine
