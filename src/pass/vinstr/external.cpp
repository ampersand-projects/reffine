#include "reffine/arrow/base.h"
#include "reffine/pass/vinstr/vinstr.h"
#include "reffine/engine/memory.h"

using namespace reffine;

ArrowArray* make_vector(size_t mem_id)
{
    return vector_builders[mem_id]();
}
