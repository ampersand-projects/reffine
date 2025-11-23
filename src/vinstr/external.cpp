#include "reffine/engine/memory.h"
#include "reffine/vinstr/vinstr.h"

using namespace reffine;

ArrowTable* make_vector(int64_t len, uint32_t mem_id)
{
    return memman.get_table(mem_id, len);
}

ArrowTable* build_vector_index(ArrowTable* tbl)
{
    auto tbl2 = reinterpret_cast<ArrowTable2*>(tbl);
    tbl2->build_index();
    return tbl;
}
