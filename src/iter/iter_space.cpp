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
    return make_shared<LBoundSpace>(iter, bound);
}

ISpace reffine::operator<=(ISpace iter, Expr bound)
{
    return make_shared<UBoundSpace>(iter, bound);
}

Expr IterSpace::_lower_bound() { return nullptr; }

Expr IterSpace::_upper_bound() { return nullptr; }

Expr IterSpace::_condition(Expr idx) { return nullptr; }

Expr IterSpace::_idx_to_iter(Expr idx) { return idx; }

Expr IterSpace::_iter_to_idx(Expr iter) { return iter; }

Expr IterSpace::_advance(Expr idx) { return _add(idx, _const(this->type, 1)); }

Expr VecSpace::_lower_bound() { return this->idx_to_iter(_idx(0)); }

Expr VecSpace::_upper_bound()
{
    return this->idx_to_iter(_len(this->vec) - _idx(1));
}

Expr VecSpace::_condition(Expr idx) { return _isval(this->vec, idx, 0); }

Expr VecSpace::_idx_to_iter(Expr idx)
{
    return _sel(_lt(idx, _len(this->vec)), _load(_fetch(this->vec, idx, 0)),
                _const(this->type, INF));
}

Expr VecSpace::_iter_to_idx(Expr iter) { return _locate(this->vec, iter); }

Expr VecSpace::_advance(Expr idx) { return _add(idx, _idx(1)); }

Expr SuperSpace::_lower_bound() { return this->ispace->lower_bound(); }

Expr SuperSpace::_upper_bound() { return this->ispace->upper_bound(); }

Expr SuperSpace::_condition(Expr idx) { return this->ispace->condition(idx); }

Expr SuperSpace::_idx_to_iter(Expr idx)
{
    return this->ispace->idx_to_iter(idx);
}

Expr SuperSpace::_iter_to_idx(Expr iter)
{
    return this->ispace->iter_to_idx(iter);
}

Expr SuperSpace::_advance(Expr idx) { return this->ispace->advance(idx); }

Expr LBoundSpace::_lower_bound()
{
    auto lb = this->ispace->lower_bound();
    return lb ? _max(this->bound, lb) : this->bound;
}

Expr UBoundSpace::_upper_bound()
{
    auto ub = this->ispace->upper_bound();
    return ub ? _min(this->bound, ub) : this->bound;
}

Expr JointSpace::_idx_to_iter(Expr idx)
{
    auto liter = this->left->idx_to_iter(_get(idx, 0));
    auto riter = this->right->idx_to_iter(_get(idx, 1));
    return _min(liter, riter);
}

Expr JointSpace::_iter_to_idx(Expr iter)
{
    auto lidx = this->left->iter_to_idx(iter);
    auto ridx = this->right->iter_to_idx(iter);
    return _new(vector<Expr>{lidx, ridx});
}

Expr JointSpace::_advance(Expr idx)
{
    auto lidx = _get(idx, 0);
    auto ridx = _get(idx, 1);
    auto liter = this->left->idx_to_iter(lidx);
    auto riter = this->right->idx_to_iter(ridx);
    auto new_lidx = this->left->advance(lidx);
    auto new_ridx = this->right->advance(ridx);

    return _sel(_lt(liter, riter), _new(vector<Expr>{new_lidx, ridx}),
                _sel(_lt(riter, liter), _new(vector<Expr>{lidx, new_ridx}),
                     _new(vector<Expr>{new_lidx, new_ridx})));
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
    auto lidx = _get(idx, 0);
    auto ridx = _get(idx, 1);

    auto lcond = this->left->condition(lidx);
    auto rcond = this->right->condition(ridx);
    auto cond = (lcond && rcond) ? _and(lcond, rcond) : (lcond ? lcond : rcond);

    auto liter = this->left->idx_to_iter(lidx);
    auto riter = this->left->idx_to_iter(ridx);
    return _and(_eq(liter, riter), cond);
}
