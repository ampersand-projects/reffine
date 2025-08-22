#include "reffine/arrow/abi.h"

#include <iostream>

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
