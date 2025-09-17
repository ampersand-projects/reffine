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

Expr IterSpace::_iter_cond(Expr idx) { return _true(); }

Expr IterSpace::_idx_to_iter(Expr idx) { return idx; }

Expr IterSpace::_iter_to_idx(Expr iter) { return iter; }

Expr IterSpace::_is_alive(Expr idx) { return _true(); }

Expr IterSpace::_next(Expr idx) { return _add(idx, _const(this->type, 1)); }

SymExprs IterSpace::_vec_idxs(Expr idx) { return SymExprs{}; }

SymExprs IterSpace::_extra_syms() { return SymExprs{}; }

Expr VecSpace::_lower_bound() { return this->idx_to_iter(_idx(0)); }

Expr VecSpace::_upper_bound()
{
    return this->idx_to_iter(_len(this->vec) - _idx(1));
}

Expr VecSpace::_iter_cond(Expr idx)
{
    return _and(_lt(idx, _len(this->vec)), _isval(this->vec, idx, 0));
}

Expr VecSpace::_idx_to_iter(Expr idx)
{
    return _load(_fetch(this->vec, idx, 0));
}

Expr VecSpace::_iter_to_idx(Expr iter) { return _locate(this->vec, iter); }

Expr VecSpace::_is_alive(Expr idx)
{
    // Loop boundary needs to be explicitly assigned to a variable
    // to help with vectorization
    return _lt(idx, this->_vec_len_sym);
}

Expr VecSpace::_next(Expr idx) { return _add(idx, _idx(1)); }

SymExprs VecSpace::_vec_idxs(Expr idx)
{
    return SymExprs{make_pair(this->vec, idx)};
}

SymExprs VecSpace::_extra_syms()
{
    return SymExprs{make_pair(this->_vec_len_sym, _len(this->vec))};
}

Expr SuperSpace::_lower_bound() { return this->ispace->lower_bound(); }

Expr SuperSpace::_upper_bound() { return this->ispace->upper_bound(); }

Expr SuperSpace::_iter_cond(Expr idx) { return this->ispace->iter_cond(idx); }

Expr SuperSpace::_idx_to_iter(Expr idx)
{
    return this->ispace->idx_to_iter(idx);
}

Expr SuperSpace::_iter_to_idx(Expr iter)
{
    return this->ispace->iter_to_idx(iter);
}

Expr SuperSpace::_is_alive(Expr idx) { return this->ispace->is_alive(idx); }

Expr SuperSpace::_next(Expr idx) { return this->ispace->next(idx); }

SymExprs SuperSpace::_vec_idxs(Expr idx) { return this->ispace->vec_idxs(idx); }

SymExprs SuperSpace::_extra_syms() { return this->ispace->extra_syms(); }

Expr LBoundSpace::_lower_bound()
{
    auto lb = this->ispace->lower_bound();
    return lb ? _max(this->bound, lb) : this->bound;
}

Expr LBoundSpace::_iter_cond(Expr idx)
{
    return _and(_gte(this->idx_to_iter(idx), this->lower_bound()),
                this->ispace->iter_cond(idx));
}

Expr UBoundSpace::_upper_bound()
{
    auto ub = this->ispace->upper_bound();
    return ub ? _min(this->bound, ub) : this->bound;
}

Expr UBoundSpace::_iter_cond(Expr idx)
{
    return _and(_lte(this->idx_to_iter(idx), this->upper_bound()),
                this->ispace->iter_cond(idx));
}

Expr UBoundSpace::_is_alive(Expr idx)
{
    return _and(_lte(this->idx_to_iter(idx), this->upper_bound()),
                this->ispace->is_alive(idx));
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

Expr JointSpace::_next(Expr idx)
{
    auto lidx = _get(idx, 0);
    auto ridx = _get(idx, 1);
    auto liter = this->left->idx_to_iter(lidx);
    auto riter = this->right->idx_to_iter(ridx);
    auto new_lidx = this->left->next(lidx);
    auto new_ridx = this->right->next(ridx);

    return _sel(_lt(liter, riter), _new(vector<Expr>{new_lidx, ridx}),
                _sel(_lt(riter, liter), _new(vector<Expr>{lidx, new_ridx}),
                     _new(vector<Expr>{new_lidx, new_ridx})));
}

SymExprs JointSpace::_vec_idxs(Expr idx)
{
    SymExprs vec_idxs;

    auto l_vec_idxs = this->left->vec_idxs(_get(idx, 0));
    auto r_vec_idxs = this->right->vec_idxs(_get(idx, 1));
    vec_idxs.insert(vec_idxs.end(), l_vec_idxs.begin(), l_vec_idxs.end());
    vec_idxs.insert(vec_idxs.end(), r_vec_idxs.begin(), r_vec_idxs.end());

    return vec_idxs;
}

SymExprs JointSpace::_extra_syms()
{
    SymExprs extra_syms;

    auto l_extra_syms = this->left->extra_syms();
    auto r_extra_syms = this->right->extra_syms();
    extra_syms.insert(extra_syms.end(), l_extra_syms.begin(),
                      l_extra_syms.end());
    extra_syms.insert(extra_syms.end(), r_extra_syms.begin(),
                      r_extra_syms.end());

    return extra_syms;
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

Expr UnionSpace::_iter_cond(Expr idx)
{
    auto lcond = this->left->iter_cond(_get(idx, 0));
    auto rcond = this->right->iter_cond(_get(idx, 1));
    return _or(lcond, rcond);
}

Expr UnionSpace::_is_alive(Expr idx)
{
    auto l_is_alive = this->left->is_alive(_get(idx, 0));
    auto r_is_alive = this->right->is_alive(_get(idx, 1));
    return _or(l_is_alive, r_is_alive);
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

Expr InterSpace::_iter_cond(Expr idx)
{
    auto lidx = _get(idx, 0);
    auto ridx = _get(idx, 1);

    auto liter = this->left->idx_to_iter(lidx);
    auto riter = this->right->idx_to_iter(ridx);
    auto iter_cond = _eq(liter, riter);

    auto lcond = this->left->iter_cond(lidx);
    auto rcond = this->right->iter_cond(ridx);

    return _and(_and(lcond, rcond), iter_cond);
}

Expr InterSpace::_is_alive(Expr idx)
{
    auto l_is_alive = this->left->is_alive(_get(idx, 0));
    auto r_is_alive = this->right->is_alive(_get(idx, 1));
    return _and(l_is_alive, r_is_alive);
}
