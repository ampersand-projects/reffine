#include "reffine/base/type.h"

using namespace reffine;

DataType types::VECTOR(DataType type)
{
    return DataType(BaseType::VECTOR, { type });
}
