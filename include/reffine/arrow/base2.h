#ifndef INCLUDE_REFFINE_ARROW_BASE2_H_
#define INCLUDE_REFFINE_ARROW_BASE2_H_

#include <cstring>
#include <vector>
#include <memory>

#include "reffine/arrow/base.h"
#include "reffine/base/log.h"
#include "reffine/base/type.h"

using namespace std;

namespace reffine {

struct ArrowSchema2 : public ArrowSchema {
    vector<ArrowSchema2*> allocs;

    ArrowSchema2(ArrowSchema& schema)
    {
        this->format = schema.format;
        this->name = schema.name;
        this->metadata = schema.metadata;
        this->flags = schema.flags;
        this->n_children = schema.n_children;
        this->children = schema.children;
        this->dictionary = schema.dictionary;
        this->release = schema.release;
        this->private_data = schema.private_data;
    }

    ArrowSchema2(string name, string format)
    {
        arrow_make_schema(this);

        strcpy((char*)this->format, format.c_str());
        strcpy((char*)this->name, name.c_str());
        this->flags = ARROW_FLAG_NULLABLE;

        this->release = (void (*)(ArrowSchema*)) & arrow_release_schema2;
    }

    static void arrow_release_schema2(ArrowSchema2* schema)
    {
        arrow_release_schema(schema);

        for (auto* alloc : schema->allocs) {
            delete alloc;
        }
        schema->allocs.clear();
    }

    ~ArrowSchema2() { if (this->release) { this->release(this); } }

    template <typename T, typename... Args>
    T* add_child(Args... args)
    {
        auto* schema = new T(args...);
        arrow_add_child_schema(this, schema);
        this->allocs.push_back(schema);

        return schema;
    }

    ArrowSchema* get_child(int idx)
    {
        return arrow_get_child_schema(this, idx);
    }
};

struct ArrowArray2 : public ArrowArray {
    vector<ArrowArray2*> allocs;

    ArrowArray2(ArrowArray& array)
    {
        this->length = array.length;
        this->null_count = array.null_count;
        this->offset = array.offset;
        this->n_buffers = array.n_buffers;
        this->n_children = array.n_children;
        this->buffers = array.buffers;
        this->children = array.children;
        this->dictionary = array.dictionary;
        this->release = array.release;
        this->private_data = array.private_data;
    }

    ArrowArray2()
    {
        arrow_make_array(this);
        this->release = (void (*)(ArrowArray*)) & arrow_release_array2;
    }

    static void arrow_release_array2(ArrowArray2* array)
    {
        arrow_release_array(array);

        for (auto* alloc : array->allocs) {
            delete alloc;
        }
        array->allocs.clear();
    }

    virtual ~ArrowArray2() { if (this->release) { this->release(this); } }

    template <typename T, typename... Args>
    T* add_child(Args... args)
    {
        auto* array = new T(args...);
        arrow_add_child_array(this, array);
        this->allocs.push_back(array);

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

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ARROW_BASE2_H_
