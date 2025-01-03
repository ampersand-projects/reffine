#ifndef INCLUDE_REFFINE_ARROW_BASE_H_
#define INCLUDE_REFFINE_ARROW_BASE_H_

#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "reffine/arrow/abi.h"

extern "C" {

struct ArrowTable {
    ArrowSchema schema;
    ArrowArray array;

    ArrowTable(ArrowSchema schema, ArrowArray array)
        : schema(std::move(schema)), array(std::move(array))
        {
        }
};

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

#endif  // INCLUDE_REFFINE_ARROW_BASE_H_
