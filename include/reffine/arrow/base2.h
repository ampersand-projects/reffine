#ifndef INCLUDE_REFFINE_ARROW_BASE2_H_
#define INCLUDE_REFFINE_ARROW_BASE2_H_

#include <cstring>
#include <vector>

#include "reffine/arrow/base.h"
#include "reffine/base/log.h"
#include "reffine/base/type.h"

using namespace std;

namespace reffine {

struct ArrowSchema2 : public ArrowSchema {
    static void arrow_init_schema2(ArrowSchema2* schema)
    {
        schema->format = nullptr;
        schema->name = nullptr;
        schema->metadata = nullptr;
        schema->flags = ARROW_FLAG_NULLABLE;
        schema->n_children = 0;
        schema->children = nullptr;
        schema->dictionary = nullptr;
        schema->release = nullptr;
        schema->private_data = nullptr;
    }

    static void arrow_release_schema2(ArrowSchema2* schema)
    {
        for (size_t i = 0; i < schema->n_children; i++) {
            auto child = (ArrowSchema2*) schema->children[i];
            child->release(child);
            delete child;
        }
        arrow_init_schema2(schema);
    }

    ArrowSchema2() { arrow_init_schema2(this); }

    ArrowSchema2(string name, string format) : _format(format), _name(name), _children()
    {
        arrow_init_schema2(this);

        this->format = this->_format.c_str();
        this->name = this->_name.c_str();
        this->release = (void (*)(ArrowSchema*)) & arrow_release_schema2;
    }

    virtual ~ArrowSchema2() { arrow_release_schema2(this); }

    template <typename T, typename... Args>
    T* add_child(Args... args)
    {
        auto* schema = new T(args...);
        this->_children.push_back(schema);
        this->children = (ArrowSchema**) this->_children.data();
        this->n_children = this->_children.size();
        return schema;
    }

private:
    string _format;
    string _name;
    vector<ArrowSchema2*> _children;
};

struct ArrowArray2 : public ArrowArray {
    static void arrow_init_array2(ArrowArray2* array)
    {
        array->length = 0;
        array->null_count = 0;
        array->offset = 0;
        array->n_buffers = 0;
        array->n_children = 0;
        array->buffers = nullptr;
        array->children = nullptr;
        array->dictionary = nullptr;
        array->release = nullptr;
        array->private_data = nullptr;
    }

    static void arrow_release_array2(ArrowArray2* array)
    {
        for (int i = 0; i < array->n_children; i++) {
            auto child = (ArrowArray2*) array->children[i];
            child->release(child);
            delete child;
        }

        for (int i = 0; i < array->n_buffers; i++) {
            free((void*) array->buffers[i]);
        }

        arrow_init_array2(array);
    }

    ArrowArray2() { arrow_init_array2(this); }

    ~ArrowArray2() { arrow_release_array2(this); }

    template <typename T, typename... Args>
    T* add_child(Args... args)
    {
        auto* array = new T(args...);
        this->_children.push_back(array);
        this->children = (ArrowArray**) this->_children.data();
        this->n_children = this->_children.size();
        return array;
    }

    template <typename T>
    T* add_buffer(size_t len)
    {
        auto buf = malloc(len * sizeof(T));
        this->_buffers.push_back(buf);
        this->buffers = (const void**) this->_buffers.data();
        this->n_buffers = this->_buffers.size();
        return (T*) buf;
    }

    template <typename T>
    T* get_buffer(size_t idx)
    {
        return (T*) this->buffers[idx];
    }

private:
    vector<void*> _buffers;
    vector<ArrowArray2*> _children;
};

struct ArrowTable {
    ArrowSchema schema;
    ArrowArray array;

    ArrowTable(ArrowSchema schema, ArrowArray array)
        : schema(std::move(schema)), array(std::move(array))
    {
        auto fmt = std::string(schema.format);
        ASSERT(fmt == "+s");
    }

    DataType get_data_type(size_t dim)
    {
        vector<DataType> dtypes;

        for (size_t i = 0; i < this->schema.n_children; i++) {
            auto child = schema.children[i];
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
            } else {
                throw std::runtime_error("schema type not supported");
            }
        }

        return DataType(BaseType::VECTOR, dtypes, dim);
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ARROW_BASE2_H_
