#ifndef INCLUDE_REFFINE_ARROW_BASE2_H_
#define INCLUDE_REFFINE_ARROW_BASE2_H_

#include <cstring>

#include "reffine/arrow/base.h"
#include "reffine/base/log.h"
#include "reffine/base/type.h"

namespace reffine {

struct PrivateData {
    int count;
    void* ptrs[100];

    PrivateData() : count(0) {}
};

struct ArrowSchema2 : public ArrowSchema {
    ArrowSchema2(std::string name, std::string format)
    {
        arrow_make_schema(this);

        strcpy((char*)this->format, format.c_str());
        strcpy((char*)this->name, name.c_str());
        this->flags = ARROW_FLAG_NULLABLE;

        this->private_data = (void*)new PrivateData();
        this->release = (void (*)(ArrowSchema*)) & arrow_release_schema2;
    }

    static void arrow_release_schema2(ArrowSchema2* schema)
    {
        if (!schema->release) return;

        arrow_release_schema(schema);

        auto* pdata = (PrivateData*)schema->private_data;
        for (int i = 0; i < pdata->count; i++) {
            delete (ArrowSchema2*)pdata->ptrs[i];
        }
        delete pdata;
    }

    ~ArrowSchema2() { arrow_release_schema2(this); }

    template <typename T, typename... Args>
    T* add_child(Args... args)
    {
        auto* schema = new T(args...);
        arrow_add_child_schema(this, schema);

        auto* pdata = (PrivateData*)this->private_data;
        pdata->ptrs[pdata->count] = schema;
        pdata->count++;

        return schema;
    }

    ArrowSchema* get_child(int idx)
    {
        return arrow_get_child_schema(this, idx);
    }
};

struct ArrowArray2 : public ArrowArray {
    ArrowArray2()
    {
        arrow_make_array(this);
        this->private_data = (void*)new PrivateData();
        this->release = (void (*)(ArrowArray*)) & arrow_release_array2;
    }

    static void arrow_release_array2(ArrowArray2* array)
    {
        if (!array->release) return;

        arrow_release_array(array);

        auto* pdata = (PrivateData*)array->private_data;
        for (int i = 0; i < pdata->count; i++) {
            delete (ArrowArray2*)pdata->ptrs[i];
        }
        delete pdata;
    }

    ~ArrowArray2() { arrow_release_array2(this); }

    template <typename T, typename... Args>
    T* add_child(Args... args)
    {
        auto* array = new T(args...);
        arrow_add_child_array(this, array);

        auto* pdata = (PrivateData*)this->private_data;
        pdata->ptrs[pdata->count] = (void*)array;
        pdata->count++;

        return array;
    }

    template <typename T>
    T* add_buffer(size_t len)
    {
        return (T*)arrow_add_buffer(this, len * sizeof(T));
    }

    template <typename T>
    T* get_child(int idx)
    {
        return (T*)arrow_get_child_array(this, idx);
    }

    template <typename T>
    T* get_buffer(int idx)
    {
        return (T*)arrow_get_buffer(this, idx);
    }
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
