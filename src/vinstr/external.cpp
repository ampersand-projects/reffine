#include "reffine/vinstr/vinstr.h"
#include "reffine/engine/memory.h"

using namespace reffine;

ArrowArray* make_vector(uint32_t mem_id)
{
    return vector_builders[mem_id]();
}
