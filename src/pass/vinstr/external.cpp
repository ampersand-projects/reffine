#include "reffine/pass/vinstr/vinstr.h"
#include "reffine/arrow/base.h"

using namespace reffine;

ArrowArray* make_vector()
{
    size_t len = 10000;

    auto* arr = new VectorArray(len);
    arr->add_child(new Int64Array(len));

    return arr;
}
