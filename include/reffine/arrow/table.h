#ifndef INCLUDE_REFFINE_ARROW_DEFS_H_
#define INCLUDE_REFFINE_ARROW_DEFS_H_

#include <unordered_map>

#include "reffine/arrow/base.h"
#include "reffine/base/log.h"
#include "reffine/base/type.h"
#include "reffine/vinstr/vinstr.h"

namespace reffine {

using IndexTy = std::unordered_map<int64_t, int64_t>;

struct ArrowTable2 : public ArrowTable {
    ArrowTable2(int64_t dim)
        : ArrowTable(dim),
          _schema(make_shared<ArrowSchema2>()),
          _array(make_shared<ArrowArray2>())
    {
        init();
    }

    ArrowTable2(std::string name, int64_t dim, size_t len,
                std::vector<std::string> cols,
                std::vector<DataType> dtypes)
        : ArrowTable(dim),
          _schema(make_shared<VectorSchema>(name)),
          _array(make_shared<VectorArray>(len))
    {
        ASSERT(cols.size() == dtypes.size());
        for (size_t i = 0; i < cols.size(); i++) {
            auto& col = cols[i];
            auto& dtype = dtypes[i];

            if (dtype == types::BOOL || dtype == types::INT8) {
                this->_schema->add_child(new Int8Schema(col));
                this->_array->add_child(new Int8Array(len));
            } else if (dtype == types::INT16) {
                this->_schema->add_child(new Int16Schema(col));
                this->_array->add_child(new Int16Array(len));
            } else if (dtype == types::INT32) {
                this->_schema->add_child(new Int32Schema(col));
                this->_array->add_child(new Int32Array(len));
            } else if (dtype == types::INT64) {
                this->_schema->add_child(new Int64Schema(col));
                this->_array->add_child(new Int64Array(len));
            } else if (dtype == types::FLOAT32) {
                this->_schema->add_child(new FloatSchema(col));
                this->_array->add_child(new FloatArray(len));
            } else if (dtype == types::FLOAT64) {
                this->_schema->add_child(new DoubleSchema(col));
                this->_array->add_child(new DoubleArray(len));
            } else {
                throw std::runtime_error("data type not supported " +
                                         dtype.str());
            }
        }

        init();
    }

    ~ArrowTable2() override {}

    DataType get_data_type()
    {
        vector<DataType> dtypes;
        vector<EncodeType> etypes;

        for (long i = 0; i < this->_schema->n_children; i++) {
            auto child = this->_schema->children[i];
            auto fmt = std::string(schema->format);
            auto dtype = this->arrow_to_dtype(child);

            dtypes.push_back(this->arrow_to_dtype(child));
            etypes.push_back((fmt == "+r") ? EncodeType::RUNEND : EncodeType::FLAT);
        }

        return DataType(BaseType::VECTOR, dtypes, this->dim, etypes);
    }

    DataType arrow_to_dtype(ArrowSchema* schema)
    {
        auto fmt = std::string(schema->format);

        if (fmt == "c") {
            return types::INT8;
        } else if (fmt == "s") {
            return types::INT16;
        } else if (fmt == "i") {
            return types::INT32;
        } else if (fmt == "l") {
            return types::INT64;
        } else if (fmt == "f") {
            return types::FLOAT32;
        } else if (fmt == "g") {
            return types::FLOAT64;
        } else if (fmt == "u") {
            return types::STR;
        } else if (fmt == "+r") {
            return arrow_to_dtype(schema->children[1]);
        } else {
            throw std::runtime_error("schema type not supported " + fmt);
        }
    }

    void build_index()
    {
        size_t col = 0;
        auto dtype = this->get_data_type().dtypes[col];
        auto etype = this->get_data_type().encodings[col];
        if (dtype == types::INT64) {
            ArrowArray* arr = nullptr;
            if (etype == EncodeType::FLAT) {
                arr = get_array_child(get_vector_array(this), col);
            } else if (etype == EncodeType::RUNEND) {
                arr = get_array_child(get_array_child(get_vector_array(this), col), 1);
            } else {
                throw runtime_error("Unknown encode type in indexing");
            }
            auto size = get_array_len(arr);
            this->index() = make_shared<IndexTy>(size);

            auto* bit_buf = (uint16_t*) get_array_buf(arr, 0);
            auto* data_buf = (int64_t*) get_array_buf(arr, 1);

            for (int64_t i = 0; i < size; i++) {
                if (get_null_bit(bit_buf, i)) {
                    this->index()->emplace(data_buf[i], i);
                }
            }
        } else {
            throw runtime_error("Data type not supported for indexing: " +
                    dtype.str());
        }
    }

    shared_ptr<IndexTy>& index() { return this->_index; }

private:
    void init()
    {
        this->schema = this->_schema.get();
        this->array = this->_array.get();
    }

    shared_ptr<ArrowSchema2> _schema;
    shared_ptr<ArrowArray2> _array;
    shared_ptr<IndexTy> _index;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ARROW_DEFS_H_
