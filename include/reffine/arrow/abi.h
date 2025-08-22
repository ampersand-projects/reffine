#ifndef INCLUDE_REFFINE_ARROW_ABI_H_
#define INCLUDE_REFFINE_ARROW_ABI_H_

#include <arrow/c/abi.h>

extern "C" {

void arrow_print_schema(ArrowSchema*);
void arrow_print_array(ArrowArray*);

}  // extern "C"

namespace reffine {

struct ArrowTableBase {
    ArrowSchema* schema;
    ArrowArray* array;

    ArrowTableBase() {}
    ArrowTableBase(ArrowSchema* schema, ArrowArray* array) : schema(schema), array(array) {}
};

} // namespace reffine

#endif  // INCLUDE_REFFINE_ARROW_ABI_H_
