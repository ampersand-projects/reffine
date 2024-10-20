#include "base2.h"

/*
 * Schema definitions
 */
template<char fmt>
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
