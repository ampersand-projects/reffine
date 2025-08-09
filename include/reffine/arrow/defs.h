#ifndef INCLUDE_REFFINE_ARROW_DEFS_H_
#define INCLUDE_REFFINE_ARROW_DEFS_H_

#include "reffine/arrow/base.h"
#include "reffine/base/type.h"

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
    NullableArray(size_t len) : ArrowArray2(len)
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

struct ArrowTable {
    ArrowSchema2 schema;
    ArrowArray2 array;

    ArrowTable() {}

    ArrowTable(std::string name, size_t len, std::vector<std::string> cols,
               std::vector<DataType> dtypes)
        : schema(name, "+s"), array(len)
    {
        this->array.add_buffer<char>(len / 8 + 1);

        ASSERT(cols.size() == dtypes.size());
        for (size_t i = 0; i < cols.size(); i++) {
            auto& col = cols[i];
            auto& dtype = dtypes[i];

            if (dtype == types::BOOL || dtype == types::INT8) {
                this->schema.add_child(new Int8Schema(col));
                this->array.add_child(new Int8Array(len));
            } else if (dtype == types::INT16) {
                this->schema.add_child(new Int16Schema(col));
                this->array.add_child(new Int16Array(len));
            } else if (dtype == types::INT32) {
                this->schema.add_child(new Int32Schema(col));
                this->array.add_child(new Int32Array(len));
            } else if (dtype == types::INT64) {
                this->schema.add_child(new Int64Schema(col));
                this->array.add_child(new Int64Array(len));
            } else if (dtype == types::FLOAT32) {
                this->schema.add_child(new FloatSchema(col));
                this->array.add_child(new FloatArray(len));
            } else if (dtype == types::FLOAT64) {
                this->schema.add_child(new DoubleSchema(col));
                this->array.add_child(new DoubleArray(len));
            } else {
                throw std::runtime_error("data type not supported " + dtype.str());
            }
        }
    }

    DataType get_data_type(size_t dim)
    {
        vector<DataType> dtypes;

        for (long i = 0; i < this->schema.n_children; i++) {
            auto child = this->schema.children[i];
            auto fmt = std::string(child->format);

            if (fmt == "c") {
                dtypes.push_back(types::INT8);
            } else if (fmt == "s") {
                dtypes.push_back(types::INT16);
            } else if (fmt == "i") {
                dtypes.push_back(types::INT32);
            } else if (fmt == "l") {
                dtypes.push_back(types::INT64);
            } else if (fmt == "f") {
                dtypes.push_back(types::FLOAT32);
            } else if (fmt == "g") {
                dtypes.push_back(types::FLOAT64);
            } else if (fmt == "u") {
                dtypes.push_back(types::STR);
            } else {
                throw std::runtime_error("schema type not supported " + fmt);
            }
        }

        return DataType(BaseType::VECTOR, dtypes, dim);
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ARROW_DEFS_H_
