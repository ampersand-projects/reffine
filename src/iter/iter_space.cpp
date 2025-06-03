#include "reffine/iter/iter_space.h"
#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

Expr VecSpace::lower_bound()
{
    return _lookup(this->vec, _idx(0));
}

Expr VecSpace::upper_bound()
{
    return _lookup(this->vec, _len(this->vec) - _idx(1));
}
