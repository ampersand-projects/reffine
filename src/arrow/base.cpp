#include "reffine/arrow/base.h"

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
