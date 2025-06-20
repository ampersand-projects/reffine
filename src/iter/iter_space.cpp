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

Expr IterSpace::_lower_bound() { return nullptr; }

Expr IterSpace::_upper_bound() { return nullptr; }

Expr IterSpace::_init_index()
{
    return _idx(0);
}

Expr IterSpace::_condition(Expr idx) { return nullptr; }

Expr IterSpace::_idx_to_iter(Expr idx)
{
    return _cast(this->type, idx);
}

Expr IterSpace::_advance(Expr idx)
{
    return _add(idx, _idx(1));
}

Expr VecSpace::_lower_bound()
{
    return _lookup(this->vec, _idx(0));
}

Expr VecSpace::_upper_bound()
{
    return _lookup(this->vec, _len(this->vec) - _idx(1));
}

Expr VecSpace::_init_index()
{
    return _idx(0);
}

Expr VecSpace::_condition(Expr idx)
{
    return _isval(this->vec, idx, 0);
}

Expr VecSpace::_idx_to_iter(Expr idx)
{
    return _lookup(this->vec, idx);
}

Expr VecSpace::_advance(Expr idx)
{
    return _add(idx, _idx(1));
}

Expr LBoundSpace::_lower_bound()
{
    return this->bound;
}

Expr UBoundSpace::_upper_bound()
{
    return this->bound;
}

Expr JointSpace::_init_index()
{
    return _new(vector<Expr>{this->left->init_index(), this->right->init_index()});
}

Expr JointSpace::_idx_to_iter(Expr idx)
{
    auto liter = this->left->idx_to_iter(_get(idx, 0));
    auto riter = this->right->idx_to_iter(_get(idx, 1));
    return _min(liter, riter);
}

Expr JointSpace::_advance(Expr idx)
{
    auto lidx = _get(idx, 0);
    auto ridx = _get(idx, 1);
    auto liter = this->left->idx_to_iter(lidx);
    auto riter = this->right->idx_to_iter(ridx);
    auto new_lidx = this->left->advance(lidx);
    auto new_ridx = this->right->advance(ridx);

    return _sel(
        _lt(liter, riter),
        _new(vector<Expr>{new_lidx, ridx}),
        _sel(
            _lt(riter, liter),
            _new(vector<Expr>{lidx, new_ridx}),
            _new(vector<Expr>{new_lidx, new_ridx})
        )
    );
}

Expr UnionSpace::_lower_bound()
{
    auto llb = this->left->lower_bound();
    auto rlb = this->right->lower_bound();
    return (llb && rlb) ? _min(llb, rlb) : (llb ? llb : rlb);
}

Expr UnionSpace::_upper_bound()
{
    auto lub = this->left->upper_bound();
    auto rub = this->right->upper_bound();
    return (lub && rub) ? _max(lub, rub) : (lub ? lub : rub);
}

Expr UnionSpace::_condition(Expr idx)
{
    auto lcond = this->left->condition(_get(idx, 0));
    auto rcond = this->right->condition(_get(idx, 1));
    return (lcond && rcond) ? _or(lcond, rcond) : (lcond ? lcond : rcond);
}

Expr InterSpace::_lower_bound()
{
    auto llb = this->left->lower_bound();
    auto rlb = this->right->lower_bound();
    return (llb && rlb) ? _max(llb, rlb) : (llb ? llb : rlb);
}

Expr InterSpace::_upper_bound()
{
    auto lub = this->left->upper_bound();
    auto rub = this->right->upper_bound();
    return (lub && rub) ? _min(lub, rub) : (lub ? lub : rub);
}

Expr InterSpace::_condition(Expr idx)
{
    auto lcond = this->left->condition(_get(idx, 0));
    auto rcond = this->right->condition(_get(idx, 1));
    return (lcond && rcond) ? _and(lcond, rcond) : (lcond ? lcond : rcond);
}
