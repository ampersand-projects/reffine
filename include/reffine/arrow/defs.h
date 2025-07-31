#ifndef INCLUDE_REFFINE_ARROW_DEFS_H_
#define INCLUDE_REFFINE_ARROW_DEFS_H_

#include "reffine/arrow/base.h"

namespace reffine {

/*
 * Schema definitions
 */
template <char fmt>
struct GenArrowSchema : public ArrowSchema2 {
    GenArrowSchema(std::string name) : ArrowSchema2(name, std::string{fmt}) {}
};

struct StructSchema : public ArrowSchema2 {
    StructSchema(std::string name) : ArrowSchema2(name, "+s") {}
};

using Int8Schema = GenArrowSchema<'c'>;
using Int16Schema = GenArrowSchema<'s'>;
using Int32Schema = GenArrowSchema<'i'>;
using Int64Schema = GenArrowSchema<'l'>;
using FloatSchema = GenArrowSchema<'f'>;
using DoubleSchema = GenArrowSchema<'g'>;
using BooleanSchema = GenArrowSchema<'c'>;
using VectorSchema = StructSchema;

/*
 * Array definitions
 */
struct NullableArray : public ArrowArray2 {
    NullableArray(size_t len) : ArrowArray2()
    {
        this->add_buffer<char>(len / 8 + 1);
    }

    char* get_bit_buf() { return this->get_buffer<char>(0); }
};

template <typename T>
struct PrimArray : public NullableArray {
    PrimArray(size_t len) : NullableArray(len) { this->add_buffer<T>(len); }

    T* get_val_buf() { return this->get_buffer<T>(1); }
};

struct StructArray : public NullableArray {
    StructArray(size_t len) : NullableArray(len) {}
};

using Int8Array = PrimArray<int8_t>;
using Int16Array = PrimArray<int16_t>;
using Int32Array = PrimArray<int32_t>;
using Int64Array = PrimArray<int64_t>;
using FloatArray = PrimArray<float>;
using DoubleArray = PrimArray<double>;
using BooleanArray = PrimArray<int8_t>;
using VectorArray = StructArray;

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ARROW_DEFS_H_
