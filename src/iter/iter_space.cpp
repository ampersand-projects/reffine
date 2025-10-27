#include "reffine/iter/iter_space.h"

#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

ISpace reffine::operator&(ISpace left, ISpace right)
{
    auto l_on_r = left->intersect(right);
    auto r_on_l = right->intersect(left);
    auto l_and_r = make_shared<InterSpace>(left, right);
    return l_on_r ? l_on_r : (r_on_l ? r_on_l : l_and_r);
}

ISpace reffine::operator|(ISpace left, ISpace right)
{
    return make_shared<UnionSpace>(left, right);
}

ISpace IterSpace::intersect(ISpace ispace) { return nullptr; }

bool IterSpace::is_const() { return false; }

Expr IterSpace::_lower_bound() { return nullptr; }

Expr IterSpace::_upper_bound() { return nullptr; }

Expr IterSpace::_iter_cond(Expr idx) { return _true(); }

Expr IterSpace::_idx_to_iter(Expr idx) { return idx; }

Expr IterSpace::_iter_to_idx(Expr iter) { return iter; }

Expr IterSpace::_is_alive(Expr idx) { return _true(); }

Expr IterSpace::_next(Expr idx) { return _add(idx, _const(idx->type, 1)); }

VecIterIdxs IterSpace::_vec_iter_idxs(Expr idx) { return VecIterIdxs{}; }

SymExprs IterSpace::_extra_syms() { return SymExprs{}; }

ISpace UniversalSpace::intersect(ISpace ispace) { return ispace; }

bool ConstantSpace::is_const() { return true; }

ISpace ConstantSpace::intersect(ISpace ispace) { return nullptr; }

Expr ConstantSpace::_lower_bound() { return this->iter; }

Expr ConstantSpace::_upper_bound() { return this->iter; }

Expr ConstantSpace::_next(Expr idx) { return idx; }

Expr VecSpace::_lower_bound() { return this->idx_to_iter(_idx(0)); }

Expr VecSpace::_upper_bound()
{
    return this->idx_to_iter(_sub(this->_vec_len_sym, _idx(1)));
}

Expr VecSpace::_iter_cond(Expr idx)
{
    auto isval = _readbit(this->vec, idx, 0);
    // need to define a symbol for isval to allow vectorization
    auto var = _define(isval->symify("valid"), isval);
    return _and(_lt(idx, this->_vec_len_sym), var);
}

Expr VecSpace::_idx_to_iter(Expr idx) { return _readdata(this->vec, idx, 0); }

Expr VecSpace::_iter_to_idx(Expr iter) { return _locate(this->vec, iter); }

Expr VecSpace::_is_alive(Expr idx)
{
    // Loop boundary needs to be explicitly assigned to a variable
    // to help with vectorization
    return _lt(idx, this->_vec_len_sym);
}

Expr VecSpace::_next(Expr idx) { return _add(idx, _idx(1)); }

VecIterIdxs VecSpace::_vec_iter_idxs(Expr idx)
{
    return VecIterIdxs{make_tuple(this->vec, this->iter, idx)};
}

SymExprs VecSpace::_extra_syms()
{
    return SymExprs{make_pair(this->_vec_len_sym, _len(this->vec, 0))};
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

VecIterIdxs SuperSpace::_vec_iter_idxs(Expr idx)
{
    return this->ispace->vec_iter_idxs(idx);
}

SymExprs SuperSpace::_extra_syms() { return this->ispace->extra_syms(); }

Expr LBoundSpace::_lower_bound()
{
    auto lb = this->ispace->lower_bound();
    return lb ? _max(this->bound->iter, lb) : this->bound->iter;
}

Expr LBoundSpace::_iter_cond(Expr idx)
{
    return _and(_gte(this->idx_to_iter(idx), this->lower_bound()),
                this->ispace->iter_cond(idx));
}

ISpace LBoundSpace::intersect(ISpace ispace)
{
    auto applied = this->ispace->intersect(ispace);
    if (applied) {
        return make_shared<LBoundSpace>(applied, this->bound);
    } else {
        return nullptr;
    }
}

Expr UBoundSpace::_upper_bound()
{
    auto ub = this->ispace->upper_bound();
    return ub ? _min(this->bound->iter, ub) : this->bound->iter;
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

ISpace UBoundSpace::intersect(ISpace ispace)
{
    auto applied = this->ispace->intersect(ispace);
    if (applied) {
        return make_shared<UBoundSpace>(applied, this->bound);
    } else {
        return nullptr;
    }
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

    auto lcond = _lte(liter, riter);
    auto lcond_sym = lcond->symify();
    auto rcond = _lte(riter, liter);
    auto rcond_sym = rcond->symify();

    auto joincond =
        _new(vector<Expr>{_sel(_define(lcond_sym, lcond), new_lidx, lidx),
                          _sel(_define(rcond_sym, rcond), new_ridx, ridx)});

    return _initval(vector<Sym>{lcond_sym, rcond_sym}, joincond);
}

VecIterIdxs JointSpace::_vec_iter_idxs(Expr idx)
{
    VecIterIdxs vec_idxs;

    auto l_vec_idxs = this->left->vec_iter_idxs(_get(idx, 0));
    auto r_vec_idxs = this->right->vec_iter_idxs(_get(idx, 1));
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

ISpace UnionSpace::intersect(ISpace ispace)
{
    auto l_applied = this->left->intersect(ispace);
    auto r_applied = this->right->intersect(ispace);

    if (l_applied && r_applied) {
        return make_shared<UnionSpace>(l_applied, r_applied);
    } else {
        return nullptr;
    }
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

ISpace InterSpace::intersect(ISpace ispace)
{
    auto l_applied = this->left->intersect(ispace);
    auto r_applied = this->right->intersect(ispace);

    if (l_applied && r_applied) {
        return make_shared<InterSpace>(l_applied, r_applied);
    } else {
        return nullptr;
    }
}

Expr NestedSpace::_lower_bound()
{
    return _new(
        vector<Expr>{this->outer->lower_bound(), this->inner->lower_bound()});
}

Expr NestedSpace::_upper_bound()
{
    return _new(
        vector<Expr>{this->outer->upper_bound(), this->inner->upper_bound()});
}

Expr NestedSpace::_iter_cond(Expr idx)
{
    auto o_idx = _get(idx, 0);
    auto i_idx = _get(idx, 1);

    return _and(this->outer->iter_cond(o_idx), this->inner->iter_cond(i_idx));
}

Expr NestedSpace::_idx_to_iter(Expr idx)
{
    auto o_idx = _get(idx, 0);
    auto i_idx = _get(idx, 1);

    return _new(vector<Expr>{this->outer->idx_to_iter(o_idx),
                             this->inner->idx_to_iter(i_idx)});
}

Expr NestedSpace::_iter_to_idx(Expr iter)
{
    auto o_iter = _get(iter, 0);
    auto i_iter = _get(iter, 1);

    return _new(vector<Expr>{this->outer->iter_to_idx(o_iter),
                             this->inner->iter_to_idx(i_iter)});
}

Expr NestedSpace::_is_alive(Expr idx)
{
    auto o_idx = _get(idx, 0);
    auto i_idx = _get(idx, 1);

    return this->outer->is_alive(o_idx);
}

Expr NestedSpace::_next(Expr idx)
{
    auto o_idx = _get(idx, 0);
    auto i_idx = _get(idx, 1);

    auto o_next = this->outer->next(o_idx);
    auto i_next = this->inner->next(i_idx);
    auto i_start = _get(this->iter_to_idx(this->lower_bound()), 1);

    auto is_inner_active = this->inner->is_alive(i_idx);
    auto var = _define(is_inner_active->symify(), is_inner_active);

    return _sel(var, _new(vector<Expr>{o_idx, i_next}),
                _new(vector<Expr>{o_next, i_start}));
}

VecIterIdxs NestedSpace::_vec_iter_idxs(Expr idx)
{
    VecIterIdxs vec_idxs;

    auto o_vec_idxs = this->outer->vec_iter_idxs(_get(idx, 0));
    auto i_vec_idxs = this->inner->vec_iter_idxs(_get(idx, 1));
    vec_idxs.insert(vec_idxs.end(), o_vec_idxs.begin(), o_vec_idxs.end());
    vec_idxs.insert(vec_idxs.end(), i_vec_idxs.begin(), i_vec_idxs.end());

    return vec_idxs;
}

SymExprs NestedSpace::_extra_syms()
{
    SymExprs extra_syms;

    auto o_extra_syms = this->outer->extra_syms();
    auto i_extra_syms = this->inner->extra_syms();
    extra_syms.insert(extra_syms.end(), o_extra_syms.begin(),
                      o_extra_syms.end());
    extra_syms.insert(extra_syms.end(), i_extra_syms.begin(),
                      i_extra_syms.end());

    return extra_syms;
}
