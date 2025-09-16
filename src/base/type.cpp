#include "reffine/base/type.h"

std::size_t std::hash<reffine::DataType>::operator()(
    const reffine::DataType& dt) const noexcept
{
    auto h = std::hash<int>()(dt.btype);
    for (auto dtype : dt.dtypes) { h ^= std::hash<reffine::DataType>()(dtype); }
    return h;
}
