#ifndef INCLUDE_REFFINE_ITER_SPACE_ITER_H_
#define INCLUDE_REFFINE_ITER_SPACE_ITER_H_

#include "reffine/base/type.h"
#include "reffine/ir/expr.h"

using namespace std;

namespace reffine {

using VecIterIdxs = vector<tuple<Expr, Expr, Expr>>;
using SymExprs = vector<pair<Sym, Expr>>;

struct IterSpace;
using ISpace = shared_ptr<IterSpace>;

struct IterSpace {
    Expr iter;

    IterSpace(Expr iter) : iter(iter) { ASSERT(iter->type.is_val()); }

    virtual ~IterSpace() {}

    Expr lower_bound()
    {
        auto lb = this->_lower_bound();
        ASSERT(!lb || lb->type == this->iter->type);
        return lb;
    }

    Expr upper_bound()
    {
        auto ub = this->_upper_bound();
        ASSERT(!ub || ub->type == this->iter->type);
        return ub;
    }

    Expr iter_cond(Expr idx)
    {
        auto cond = this->_iter_cond(idx);
        ASSERT(cond->type == types::BOOL);
        return cond;
    }

    Expr idx_to_iter(Expr idx)
    {
        auto iter = this->_idx_to_iter(idx);
        ASSERT(iter->type == this->iter->type);
        return iter;
    }

    Expr iter_to_idx(Expr iter) { return this->_iter_to_idx(iter); }

    Expr is_alive(Expr idx)
    {
        auto is_alive = this->_is_alive(idx);
        ASSERT(is_alive->type == types::BOOL);
        return is_alive;
    }

    Expr next(Expr idx)
    {
        auto new_idx = this->_next(idx);
        ASSERT(new_idx->type == idx->type);
        return new_idx;
    }

    VecIterIdxs vec_iter_idxs(Expr idx) { return this->_vec_iter_idxs(idx); }

    SymExprs extra_syms() { return this->_extra_syms(); }

    virtual ISpace intersect(ISpace);

    virtual bool is_const();

protected:
    virtual Expr _lower_bound();
    virtual Expr _upper_bound();
    virtual Expr _iter_cond(Expr);
    virtual Expr _idx_to_iter(Expr);
    virtual Expr _iter_to_idx(Expr);
    virtual Expr _is_alive(Expr);
    virtual Expr _next(Expr);
    virtual VecIterIdxs _vec_iter_idxs(Expr);
    virtual SymExprs _extra_syms();
};

struct UniversalSpace : public IterSpace {
    UniversalSpace(Sym iter) : IterSpace(iter) {}

    ISpace intersect(ISpace) final;
};

struct ConstantSpace : public IterSpace {
    ConstantSpace(Expr iter) : IterSpace(iter)
    {
    }

    ISpace intersect(ISpace) final;
    bool is_const() final;

private:
    virtual Expr _lower_bound();
    virtual Expr _upper_bound();
    virtual Expr _next(Expr);
};

struct VecSpace : public IterSpace {
    Expr vec;

    VecSpace(Expr iter, Expr vec)
        : IterSpace(iter),
          vec(vec),
          _vec_len_sym(make_shared<SymNode>(vec->str() + "_len", types::IDX))
    {
        ASSERT(vec->type.is_vector());
        ASSERT(vec->type.dim == 1);  // currently only support 1d vectors
    }

private:
    Expr _lower_bound() final;
    Expr _upper_bound() final;
    Expr _iter_cond(Expr) final;
    Expr _idx_to_iter(Expr) final;
    Expr _iter_to_idx(Expr) final;
    Expr _is_alive(Expr) final;
    Expr _next(Expr) final;
    VecIterIdxs _vec_iter_idxs(Expr) final;
    SymExprs _extra_syms() final;

    Sym _vec_len_sym;
};

struct SuperSpace : public IterSpace {
    ISpace ispace;

    SuperSpace(ISpace ispace) : IterSpace(ispace->iter), ispace(ispace) {}

protected:
    Expr _lower_bound() override;
    Expr _upper_bound() override;
    Expr _iter_cond(Expr) override;
    Expr _idx_to_iter(Expr) override;
    Expr _iter_to_idx(Expr) override;
    Expr _is_alive(Expr) override;
    Expr _next(Expr) override;
    VecIterIdxs _vec_iter_idxs(Expr) final;
    SymExprs _extra_syms() final;
};

struct BoundSpace : public SuperSpace {
    ISpace bound;

    BoundSpace(ISpace ispace, ISpace bound) : SuperSpace(ispace), bound(bound)
    {
        ASSERT(bound->is_const());
        ASSERT(bound->iter->type == ispace->iter->type);
    }
};

struct LBoundSpace : public BoundSpace {
    LBoundSpace(ISpace ispace, ISpace bound) : BoundSpace(ispace, bound) {}

    ISpace intersect(ISpace) final;

private:
    Expr _lower_bound() final;
    Expr _iter_cond(Expr) final;
};

struct UBoundSpace : public BoundSpace {
    UBoundSpace(ISpace ispace, ISpace bound) : BoundSpace(ispace, bound) {}

    ISpace intersect(ISpace) final;

private:
    Expr _upper_bound() final;
    Expr _iter_cond(Expr) final;
    Expr _is_alive(Expr) final;
};

struct JointSpace : public IterSpace {
    ISpace left;
    ISpace right;

    JointSpace(ISpace left, ISpace right)
        : IterSpace(left->iter), left(left), right(right)
    {
        ASSERT(left->iter == right->iter);
    }

protected:
    Expr _idx_to_iter(Expr) final;
    Expr _iter_to_idx(Expr) final;
    Expr _next(Expr) final;
    VecIterIdxs _vec_iter_idxs(Expr) final;
    SymExprs _extra_syms() final;
};

struct UnionSpace : public JointSpace {
    UnionSpace(ISpace left, ISpace right) : JointSpace(left, right) {}

    ISpace intersect(ISpace) final;

private:
    Expr _lower_bound() final;
    Expr _upper_bound() final;
    Expr _iter_cond(Expr) final;
    Expr _is_alive(Expr) final;
};

struct InterSpace : public JointSpace {
    InterSpace(ISpace left, ISpace right) : JointSpace(left, right) {}

    ISpace intersect(ISpace) final;

private:
    Expr _lower_bound() final;
    Expr _upper_bound() final;
    Expr _iter_cond(Expr) final;
    Expr _is_alive(Expr) final;
};

struct NestedSpace : public IterSpace {
    ISpace outer;
    ISpace inner;

    NestedSpace(ISpace outer, ISpace inner)
        : IterSpace(make_shared<New>(vector<Expr>{outer->iter, inner->iter})),
          outer(outer),
          inner(inner)
    {
    }

private:
    Expr _lower_bound() final;
    Expr _upper_bound() final;
    Expr _iter_cond(Expr) final;
    Expr _idx_to_iter(Expr) final;
    Expr _iter_to_idx(Expr) final;
    Expr _is_alive(Expr) final;
    Expr _next(Expr) final;
    VecIterIdxs _vec_iter_idxs(Expr) final;
    SymExprs _extra_syms() final;
};

ISpace operator&(ISpace, ISpace);
ISpace operator|(ISpace, ISpace);

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ITER_SPACE_ITER_H_
