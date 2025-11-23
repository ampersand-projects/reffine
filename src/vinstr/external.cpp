#include "reffine/engine/memory.h"
#include "reffine/vinstr/vinstr.h"

using namespace reffine;

ArrowTable* make_vector(int64_t len, uint32_t mem_id)
{
    return memman.get_table(mem_id, len);
}

int64_t vector_locate(ArrowTable* tbl, int64_t val)
{
    auto* tbl2 = reinterpret_cast<ArrowTable2*>(tbl);
    auto it = tbl2->index()->find(val);
    if (it != tbl2->index()->end()) {
        return it->second;
    } else {
        return -1;
    }
}

ArrowTable* build_vector_index(ArrowTable* tbl)
{
    auto tbl2 = reinterpret_cast<ArrowTable2*>(tbl);
    tbl2->build_index();
    return tbl;
}
