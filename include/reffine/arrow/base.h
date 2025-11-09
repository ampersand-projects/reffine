#ifndef INCLUDE_REFFINE_ARROW_BASE_H_
#define INCLUDE_REFFINE_ARROW_BASE_H_

#include <string>
#include <vector>

#include "reffine/arrow/abi.h"
#include "reffine/vinstr/vinstr.h"

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
            for (auto* child : this->children) { delete child; }
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
        this->release = (void (*)(ArrowSchema*))&arrow_release_schema2;
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

    ArrowSchema2* get_child(int idx) { return this->pdata()->children[idx]; }

    Private* pdata() { return (Private*)this->private_data; }
};

struct ArrowArray2 : public ArrowArray {
    using IndexTy = std::unordered_map<int64_t, int64_t>;

    struct Private {
        Private(size_t len) : children(0), buffers(0) {}

        ~Private()
        {
            for (auto* child : this->children) { delete child; }

            for (auto* buffer : this->buffers) { delete[] buffer; }
        }

        size_t len;
        vector<ArrowArray2*> children;
        vector<const char*> buffers;
        shared_ptr<IndexTy> index;
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
        this->release = (void (*)(ArrowArray*))&arrow_release_array2;
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
        this->children = (ArrowArray**)this->pdata()->children.data();
        this->n_children = this->pdata()->children.size();
    }

    template <typename T>
    T* add_buffer(size_t len)
    {
        void* buf = new char[len * sizeof(T)];
        return (T*) this->add_buffer(buf);
    }

    void* add_buffer(void* buf)
    {
        this->pdata()->buffers.push_back((char*)buf);
        this->buffers = (const void**)this->pdata()->buffers.data();
        this->n_buffers = this->pdata()->buffers.size();

        return buf;
    }

    ArrowArray2* get_child(int idx) { return this->pdata()->children[idx]; }

    template <typename T>
    T* get_buffer(int idx)
    {
        return (T*)this->pdata()->buffers[idx];
    }

    shared_ptr<IndexTy>& index()
    {
        return this->pdata()->index;
    }

    Private* pdata() { return (Private*)this->private_data; }

    template <typename T>
    void build_flat_index()
    {
        auto size = get_array_len(this);
        auto* bitmap = this->get_buffer<uint16_t>(0);
        auto* iter_col = this->get_buffer<T>(1);

        this->index() = make_shared<IndexTy>(size);

        for (int64_t i = 0; i < size; i++) {
            if (get_null_bit(bitmap, i)) {
                this->index()->emplace(iter_col[i], i);
            }
        }
    }
};

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

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ARROW_BASE_H_
