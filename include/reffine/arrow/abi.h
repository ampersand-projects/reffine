#ifndef INCLUDE_REFFINE_ARROW_ABI_H_
#define INCLUDE_REFFINE_ARROW_ABI_H_

#include <arrow/c/abi.h>

extern "C" {

void arrow_print_schema(ArrowSchema*);
void arrow_print_array(ArrowArray*);

struct ArrowTable {
    ArrowSchema* schema;
    ArrowArray* array;

    ArrowTable() {}
    ArrowTable(ArrowSchema* schema, ArrowArray* array) : schema(schema), array(array) {}

    virtual ~ArrowTable() {}
};

}  // extern "C"

#endif  // INCLUDE_REFFINE_ARROW_ABI_H_
