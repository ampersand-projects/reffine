#ifndef INCLUDE_REFFINE_ARROW_DEFS_H_
#define INCLUDE_REFFINE_ARROW_DEFS_H_

#include <unordered_map>

#include "reffine/arrow/base.h"
#include "reffine/base/type.h"
#include "reffine/base/log.h"
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
                std::vector<std::string> cols, std::vector<DataType> dtypes)
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

        for (long i = 0; i < this->_schema->n_children; i++) {
            auto child = this->_schema->children[i];
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

        return DataType(BaseType::VECTOR, dtypes, this->dim);
    }

    void build_index()
    {
        // Only support indexing for uni-dimensional veectors
        // with integer iter type
        ASSERT(this->dim == 1);
        auto dtype = this->get_data_type().iterty();
        ASSERT(dtype.is_int());

        auto size = this->array->length;
        this->index().reserve(size);
        if (dtype == types::INT64) {
            auto* iter_col = (int64_t*) get_vector_data_buf(this, 0);
            for (int64_t i = 0; i < size; i++) {
                if (get_vector_null_bit(this, i, 0)) {
                    this->index().emplace(iter_col[i], i);
                }
            }
        } else {
            throw runtime_error("Data type not supported for indexing: " + dtype.str());
        }
    }

    IndexTy& index() { return this->_index; }

private:
    void init()
    {
        this->schema = this->_schema.get();
        this->array = this->_array.get();
    }

    shared_ptr<ArrowSchema2> _schema;
    shared_ptr<ArrowArray2> _array;
    IndexTy _index;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ARROW_DEFS_H_
