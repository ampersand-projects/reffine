#include "reffine/arrow/base.h"
#include "reffine/arrow/defs.h"

using namespace reffine;

void arrow_print_schema(ArrowSchema* schema)
{
    std::cout << "===== Schema (" << std::hex << schema << std::dec
              << ") ======" << std::endl;
    std::cout << "Format: " << schema->format << std::endl;
    std::cout << "Name: " << schema->name << std::endl;
    std::cout << "Flags: " << schema->flags << " ";
    std::cout << ((schema->flags & ARROW_FLAG_NULLABLE) ? "Nullable" : "")
              << " ";
    std::cout << ((schema->flags & ARROW_FLAG_DICTIONARY_ORDERED)
                      ? "Dict ordered"
                      : "")
              << " ";
    std::cout << ((schema->flags & ARROW_FLAG_MAP_KEYS_SORTED)
                      ? "Map keys sorted"
                      : "")
              << std::endl;
    std::cout << "Num of children: " << schema->n_children << " [ ";
    for (int i = 0; i < schema->n_children; i++) {
        std::cout << std::hex << schema->children[i] << std::dec << " ";
    }
    std::cout << "]" << std::endl << std::endl;

    for (int i = 0; i < schema->n_children; i++) {
        arrow_print_schema(schema->children[i]);
    }
}

void arrow_release_schema(ArrowSchema* schema)
{
    for (int i = 0; i < schema->n_children; i++) {
        schema->children[i]->release(schema->children[i]);
    }

    free((void*)schema->format);
    free((void*)schema->name);
    free(schema->children);

    schema->release = nullptr;
}

void arrow_make_schema(ArrowSchema* schema)
{
    schema->format = (const char*)calloc(10, sizeof(char));
    schema->name = (const char*)calloc(100, sizeof(char));
    schema->metadata = nullptr;
    schema->flags = ARROW_FLAG_NULLABLE;
    schema->n_children = 0;
    schema->children = (ArrowSchema**)calloc(10, sizeof(ArrowSchema*));
    schema->dictionary = nullptr;
    schema->release = &arrow_release_schema;
    schema->private_data = nullptr;
}

void arrow_add_child_schema(ArrowSchema* parent, ArrowSchema* child)
{
    parent->children[parent->n_children] = child;
    parent->n_children++;
}

ArrowSchema* arrow_get_child_schema(ArrowSchema* schema, int idx)
{
    return schema->children[idx];
}

void arrow_print_array(ArrowArray* array)
{
    std::cout << "===== Array (" << std::hex << array << std::dec
              << ") ======" << std::endl;
    std::cout << "Length: " << array->length << std::endl;
    std::cout << "Null count: " << array->null_count << std::endl;
    std::cout << "Offset: " << array->offset << std::endl;
    std::cout << "Num of buffers: " << array->n_buffers << " [ ";
    for (int i = 0; i < array->n_buffers; i++) {
        std::cout << std::hex << array->buffers[i] << std::dec << " ";
    }
    std::cout << "]" << std::endl;

    std::cout << "Num of children: " << array->n_children << " [ ";
    for (int i = 0; i < array->n_children; i++) {
        std::cout << std::hex << array->children[i] << std::dec << " ";
    }
    std::cout << "]" << std::endl << std::endl;

    for (int i = 0; i < array->n_children; i++) {
        arrow_print_array(array->children[i]);
    }
}

void arrow_release_array(ArrowArray* array)
{
    for (int i = 0; i < array->n_buffers; i++) {
        free((void*)array->buffers[i]);
    }
    for (int i = 0; i < array->n_children; i++) {
        array->children[i]->release(array->children[i]);
    }

    free(array->buffers);
    free(array->children);

    array->release = nullptr;
}

void arrow_make_array(ArrowArray* array)
{
    array->length = 0;
    array->null_count = 0;
    array->offset = 0;
    array->n_buffers = 0;
    array->n_children = 0;
    array->buffers = (const void**)calloc(10, sizeof(void*));
    array->children = (ArrowArray**)calloc(10, sizeof(ArrowArray*));
    array->dictionary = nullptr;
    array->release = &arrow_release_array;
    array->private_data = nullptr;
}

void* arrow_add_buffer(ArrowArray* array, size_t size)
{
    auto* buf = malloc(size);

    array->buffers[array->n_buffers] = buf;
    array->n_buffers++;

    return buf;
}

void arrow_add_child_array(ArrowArray* parent, ArrowArray* child)
{
    parent->children[parent->n_children] = child;
    parent->n_children++;
}

ArrowArray* arrow_get_child_array(ArrowArray* array, int idx)
{
    return array->children[idx];
}

void* arrow_get_buffer(ArrowArray* array, int idx)
{
    return (void*)array->buffers[idx];
}

ArrowTable::ArrowTable(std::string name, std::vector<DataType> dtypes, size_t len)
    : schema(make_shared<ArrowSchema2>(name, "+s")), array(make_shared<ArrowArray2>())
{
    for (auto& dtype : dtypes) {
        ASSERT(dtype.is_primitive());

        if (dtype == types::INT8) {
            this->schema->add_child<Int8Schema>(name);
            this->array->add_child<Int8Array>(len);
        } else if (dtype == types::INT16) {
            this->schema->add_child<Int16Schema>(name);
            this->array->add_child<Int16Array>(len);
        } else if (dtype == types::INT32) {
            this->schema->add_child<Int32Schema>(name);
            this->array->add_child<Int32Array>(len);
        } else if (dtype == types::INT64) {
            this->schema->add_child<Int64Schema>(name);
            this->array->add_child<Int64Array>(len);
        } else if (dtype == types::FLOAT32) {
            this->schema->add_child<FloatSchema>(name);
            this->array->add_child<FloatArray>(len);
        } else if (dtype == types::FLOAT64) {
            this->schema->add_child<DoubleSchema>(name);
            this->array->add_child<DoubleArray>(len);
        } else if (dtype == types::BOOL) {
            this->schema->add_child<BooleanSchema>(name);
            this->array->add_child<BooleanArray>(len);
        } else {
            throw std::runtime_error("data type not supported: " + dtype.str());
        }
    }
}

DataType ArrowTable::vecty(size_t dim)
{
    vector<DataType> dtypes;

    for (int64_t i = 0; i < this->schema->n_children; i++) {
        auto child = this->schema->children[i];
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
            throw std::runtime_error("schema type not supported: " + fmt);
        }
    }

    return DataType(BaseType::VECTOR, dtypes, dim);
}
