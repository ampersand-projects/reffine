#ifndef INCLUDE_REFFINE_ARROW_DEFS_H_
#define INCLUDE_REFFINE_ARROW_DEFS_H_

#include "reffine/arrow/base.h"
#include "reffine/base/type.h"

namespace reffine {

struct ArrowTable2 : public ArrowTable {
    ArrowTable2() :
        ArrowTable(),
        schema2(make_shared<ArrowSchema2>()),
        array2(make_shared<ArrowArray2>())
    {
        init();
    }

    ArrowTable2(std::string name, size_t len, std::vector<std::string> cols,
               std::vector<DataType> dtypes)
    {
        this->schema2 = make_shared<VectorSchema>(name);
        this->array2 = make_shared<VectorArray>(len);

        ASSERT(cols.size() == dtypes.size());
        for (size_t i = 0; i < cols.size(); i++) {
            auto& col = cols[i];
            auto& dtype = dtypes[i];

            if (dtype == types::BOOL || dtype == types::INT8) {
                this->schema2->add_child(new Int8Schema(col));
                this->array2->add_child(new Int8Array(len));
            } else if (dtype == types::INT16) {
                this->schema2->add_child(new Int16Schema(col));
                this->array2->add_child(new Int16Array(len));
            } else if (dtype == types::INT32) {
                this->schema2->add_child(new Int32Schema(col));
                this->array2->add_child(new Int32Array(len));
            } else if (dtype == types::INT64) {
                this->schema2->add_child(new Int64Schema(col));
                this->array2->add_child(new Int64Array(len));
            } else if (dtype == types::FLOAT32) {
                this->schema2->add_child(new FloatSchema(col));
                this->array2->add_child(new FloatArray(len));
            } else if (dtype == types::FLOAT64) {
                this->schema2->add_child(new DoubleSchema(col));
                this->array2->add_child(new DoubleArray(len));
            } else {
                throw std::runtime_error("data type not supported " +
                                         dtype.str());
            }
        }

        init();
    }

    ~ArrowTable2() override {}

    DataType get_data_type(size_t dim)
    {
        vector<DataType> dtypes;

        for (long i = 0; i < this->schema2->n_children; i++) {
            auto child = this->schema2->children[i];
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

private:
    void init()
    {
        this->schema = this->schema2.get();
        this->array = this->array2.get();
    }

    shared_ptr<ArrowSchema2> schema2;
    shared_ptr<ArrowArray2> array2;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ARROW_DEFS_H_
