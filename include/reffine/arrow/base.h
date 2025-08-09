#ifndef INCLUDE_REFFINE_ARROW_BASE_H_
#define INCLUDE_REFFINE_ARROW_BASE_H_

#include <vector>
#include <string>
#include <arrow/c/abi.h>

extern "C" {

void arrow_print_schema(ArrowSchema*);
void arrow_print_array(ArrowArray*);

}  // extern "C"

namespace reffine {

using namespace std;

struct ArrowSchema2 : public ArrowSchema {
    struct Private {
        Private(std::string format, std::string name)
            : format(format), name(name), children(0)
        {
        }

        ~Private()
        {
            for (auto* child : this->children) {
                delete child;
            }
        }

        string format;
        string name;
        vector<ArrowSchema2*> children;
    };

    ArrowSchema2() {}

    ArrowSchema2(std::string name, std::string format)
    {
        auto pdata = new Private(format, name);

        this->format = pdata->format.c_str();
        this->name = pdata->name.c_str();
        this->metadata = nullptr;
        this->flags = ARROW_FLAG_NULLABLE;
        this->n_children = 0;
        this->children = nullptr;
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

    void add_child(ArrowSchema2* schema)
    {
        this->pdata()->children.push_back(schema);
        this->children = (ArrowSchema**)this->pdata()->children.data();
        this->n_children = this->pdata()->children.size();
    }

    ArrowSchema2* get_child(int idx)
    {
        return this->pdata()->children[idx];
    }

    Private* pdata() { return (Private*)this->private_data; }
};

struct ArrowArray2 : public ArrowArray {
    struct Private {
        Private(size_t len) : children(0), buffers(0) {}

        ~Private()
        {
            for (auto* child : this->children) {
                delete child;
            }

            for (auto* buffer : this->buffers) {
                delete [] buffer;
            }
        }

        size_t len;
        vector<ArrowArray2*> children;
        vector<const char*> buffers;
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

    void add_child(ArrowArray2* array)
    {
        this->pdata()->children.push_back(array);
        this->children = (ArrowArray**) this->pdata()->children.data();
        this->n_children = this->pdata()->children.size();
    }

    template <typename T>
    T* add_buffer(size_t len)
    {
        auto* buf = new char[len * sizeof(T)];

        this->pdata()->buffers.push_back(buf);
        this->buffers = (const void**)this->pdata()->buffers.data();
        this->n_buffers = this->pdata()->buffers.size();

        return (T*)buf;
    }

    ArrowArray2* get_child(int idx) { return this->pdata()->children[idx]; }

    template <typename T>
    T* get_buffer(int idx)
    {
        return (T*)this->pdata()->buffers[idx];
    }

    Private* pdata() { return (Private*)this->private_data; }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ARROW_BASE_H_
