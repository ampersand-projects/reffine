#include "base2.h"

/*
 * Schema definitions
 */
GEN_SCHEMA(Int8Schema, "c")
GEN_SCHEMA(Int16Schema, "s")
GEN_SCHEMA(Int32Schema, "i")
GEN_SCHEMA(Int64Schema, "l")
GEN_SCHEMA(FloatSchema, "f")
GEN_SCHEMA(DoubleSchema, "g")
GEN_SCHEMA(StructSchema, "+s")


/*
 * Array definitions
 */
using Int8Array = PrimArray<int8_t>;
using Int16Array = PrimArray<int16_t>;
using Int32Array = PrimArray<int32_t>;
using Int64Array = PrimArray<int64_t>;
using FloatArray = PrimArray<float>;
using DoubleArray = PrimArray<double>;

struct StructArray : public NullableArray {
    StructArray(size_t len) : NullableArray(len) {}
};
