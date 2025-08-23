#ifndef INCLUDE_REFFINE_ENGINE_MEMORY_H_
#define INCLUDE_REFFINE_ENGINE_MEMORY_H_

#include <functional>
#include <vector>

#include "reffine/arrow/table.h"

namespace reffine {

using VectorBuilderFnTy = std::function<shared_ptr<ArrowTable2>()>;

class MemoryManager {
public:
    uint32_t add_builder(VectorBuilderFnTy);
    ArrowTable* get_table(uint32_t);

private:
    std::vector<VectorBuilderFnTy> _builders;
    std::vector<shared_ptr<ArrowTable2>> _tables;
};

inline MemoryManager memman;

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ENGINE_MEMORY_H_
