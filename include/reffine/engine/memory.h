#ifndef INCLUDE_REFFINE_ENGINE_MEMORY_H_
#define INCLUDE_REFFINE_ENGINE_MEMORY_H_

#include <vector>
#include <functional>

#include "reffine/arrow/table.h"

namespace reffine {

using VectorBuilderFnTy = std::function<ArrowArray*()>;
inline std::vector<VectorBuilderFnTy> vector_builders{};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ENGINE_MEMORY_H_
