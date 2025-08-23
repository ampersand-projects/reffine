#include "reffine/engine/memory.h"

using namespace reffine;

uint32_t MemoryManager::add_builder(VectorBuilderFnTy fn)
{
    this->_builders.push_back(fn);
    return this->_builders.size() - 1;
}

ArrowTable* MemoryManager::get_table(uint32_t mem_id)
{
    auto tbl = this->_builders[mem_id]();
    this->_tables.push_back(tbl);
    return tbl.get();
}
