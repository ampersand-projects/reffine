#include "reffine/engine/memory.h"
#include "reffine/vinstr/vinstr.h"

using namespace reffine;

ArrowTable* make_vector(uint32_t mem_id)
{
    return memman.get_table(mem_id);
}
