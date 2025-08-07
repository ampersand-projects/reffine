#ifndef INCLUDE_REFFINE_ARROW_BASE_H_
#define INCLUDE_REFFINE_ARROW_BASE_H_

#include <arrow/c/abi.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "reffine/base/log.h"
#include "reffine/base/type.h"

extern "C" {

void arrow_print_schema(ArrowSchema*);
void arrow_release_schema(ArrowSchema*);
void arrow_make_schema(ArrowSchema*);
void arrow_add_child_schema(ArrowSchema*, ArrowSchema*);
ArrowSchema* arrow_get_child_schema(ArrowSchema*, int);

void arrow_print_array(ArrowArray*);
void arrow_release_array(ArrowArray*);
void arrow_make_array(ArrowArray*);
void* arrow_add_buffer(ArrowArray*, size_t);
void arrow_add_child_array(ArrowArray*, ArrowArray*);
ArrowArray* arrow_get_child_array(ArrowArray*, int);
void* arrow_get_buffer(ArrowArray*, int);

}  // extern "C"

namespace reffine {

using namespace std;

struct ArrowSchema2 : public ArrowSchema {
    struct Private {
        Private(std::string format, std::string name)
            : format(format), name(name), children(0), schemas(0)
        {
        }

        string format;
        string name;
        vector<ArrowSchema*> children;
        vector<shared_ptr<ArrowSchema2>> schemas;
    };

    ArrowSchema2() {}

    ArrowSchema2(std::string name, std::string format)
    {
        auto pdata = new Private(format, name);

        this->format = pdata->format.c_str();
        this->name = pdata->name.c_str();
        this->metadata = nullptr;
        this->flags = ARROW_FLAG_NULLABLE;
        this->n_children = pdata->children.size();
        this->children = pdata->children.data();
        this->dictionary = nullptr;
        this->release = (void (*)(ArrowSchema*)) & arrow_release_schema2;
        this->private_data = pdata;
    }

    static void arrow_release_schema2(ArrowSchema2* schema)
    {
        delete schema->pdata();
        schema->release = nullptr;
    }

    ~ArrowSchema2()
    {
        if (this->release) this->release(this);
    }

    void add_child(shared_ptr<ArrowSchema2> schema)
    {
        this->pdata()->schemas.push_back(schema);
        this->pdata()->children.push_back(schema.get());
        this->children = (ArrowSchema**)this->pdata()->children.data();
        this->n_children = this->pdata()->children.size();
    }

    shared_ptr<ArrowSchema2> get_child(int idx)
    {
        return this->pdata()->schemas[idx];
    }

    Private* pdata() { return (Private*)this->private_data; }
};

struct ArrowArray2 : public ArrowArray {
    struct Private {
        Private(size_t len) : children(0), arrays(0), buffers(0), buf_vecs(0) {}

        size_t len;
        vector<ArrowArray*> children;
        vector<shared_ptr<ArrowArray2>> arrays;
        vector<const void*> buffers;
        vector<vector<char>> buf_vecs;
    };

    ArrowArray2() {}

    ArrowArray2(size_t len)
    {
        auto pdata = new Private(len);

        this->length = 0;
        this->null_count = 0;
        this->offset = 0;
        this->n_buffers = 0;
        this->n_children = 0;
        this->buffers = nullptr;
        this->children = nullptr;
        this->dictionary = nullptr;
        this->release = (void (*)(ArrowArray*)) & arrow_release_array2;
        this->private_data = pdata;
    }

    static void arrow_release_array2(ArrowArray2* array)
    {
        delete array->pdata();
        array->private_data = nullptr;
        array->release = nullptr;
    }

    ~ArrowArray2()
    {
        if (this->release) { this->release(this); }
    }

    void add_child(shared_ptr<ArrowArray2> array)
    {
        this->pdata()->arrays.push_back(array);
        this->pdata()->children.push_back(array.get());
        this->children = this->pdata()->children.data();
        this->n_children = this->pdata()->children.size();
    }

    template <typename T>
    T* add_buffer(size_t len)
    {
        auto& buf = this->pdata()->buf_vecs.emplace_back(len * sizeof(T));

        this->pdata()->buffers.push_back(buf.data());
        this->buffers = (const void**)this->pdata()->buffers.data();
        this->n_buffers = this->pdata()->buffers.size();

        return (T*)buf.data();
    }

    ArrowArray2* get_child(int idx) { return this->pdata()->arrays[idx].get(); }

    template <typename T>
    T* get_buffer(int idx)
    {
        return (T*)this->pdata()->buffers[idx];
    }

    Private* pdata() { return (Private*)this->private_data; }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ARROW_BASE_H_
