#include "reffine/iter/iter_space.h"
#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

ISpace reffine::operator&(ISpace left, ISpace right)
{
    return make_shared<InterSpace>(left, right);
}

ISpace reffine::operator|(ISpace left, ISpace right)
{
    return make_shared<UnionSpace>(left, right);
}

ISpace reffine::operator>=(ISpace iter, Expr bound)
{
    return iter & make_shared<LBoundSpace>(bound);
}

ISpace reffine::operator<=(ISpace iter, Expr bound)
{
    return iter & make_shared<UBoundSpace>(bound);
}

Expr VecSpace::lower_bound()
{
    return _lookup(this->vec, _idx(0));
}

Expr VecSpace::upper_bound()
{
    return _lookup(this->vec, _len(this->vec) - _idx(1));
}

Expr LBoundSpace::lower_bound()
{
    return this->bound;
}

Expr UBoundSpace::upper_bound()
{
    return this->bound;
}

Expr UnionSpace::lower_bound()
{
    return _min(this->left->lower_bound(), this->right->upper_bound());
}

Expr UnionSpace::upper_bound()
{
    return _max(this->left->lower_bound(), this->right->upper_bound());
}

Expr InterSpace::lower_bound()
{
    return _max(this->left->lower_bound(), this->right->upper_bound());
}

Expr InterSpace::upper_bound()
{
    return _min(this->left->lower_bound(), this->right->upper_bound());
}
