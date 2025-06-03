#ifndef INCLUDE_REFFINE_ITER_SPACE_ITER_H_
#define INCLUDE_REFFINE_ITER_SPACE_ITER_H_

#include "reffine/base/type.h"
#include "reffine/ir/node.h"

using namespace std;

namespace reffine {

struct IterSpace {
    const DataType type;

    IterSpace(DataType type) : type(type) {}

    virtual ~IterSpace() {}

    virtual Expr lower_bound() { return nullptr; }
    virtual Expr upper_bound() { return nullptr; }
};
using ISpace = shared_ptr<IterSpace>;

struct VecSpace : public IterSpace {
    Sym vec;

    VecSpace(Sym vec) : IterSpace(vec->type.iterty()), vec(vec)
    {
        ASSERT(vec->type.is_vector());
        ASSERT(vec->type.dim == 1); // currently only support 1d vectors
    }

    Expr lower_bound() final;
    Expr upper_bound() final;
};

struct BoundSpace : public IterSpace {
    ISpace iter;
    Expr bound;

    BoundSpace(ISpace iter, Expr bound) : IterSpace(iter->type), iter(iter), bound(bound)
    {
        ASSERT(iter->type == bound->type);
        ASSERT(bound->type.is_val());
    }
};

struct LBoundSpace : public BoundSpace {
    LBoundSpace(ISpace iter, Expr bound) : BoundSpace(iter, bound) {}

    Expr lower_bound() final;
    Expr upper_bound() final;
};

struct UBoundSpace : public BoundSpace {
    UBoundSpace(ISpace iter, Expr bound) : BoundSpace(iter, bound) {}

    Expr lower_bound() final;
    Expr upper_bound() final;
};

struct JoinSpace : public IterSpace {
    ISpace left;
    ISpace right;

    JoinSpace(ISpace left, ISpace right) : IterSpace(left->type), left(left), right(right)
    {
        ASSERT(left->type == right->type);
    }
};

struct UnionSpace : public JoinSpace {
    UnionSpace(ISpace left, ISpace right) : JoinSpace(left, right) {}

    Expr lower_bound() override;
    Expr upper_bound() override;
};

struct InterSpace : public JoinSpace {
    InterSpace(ISpace left, ISpace right) : JoinSpace(left, right) {}

    Expr lower_bound() override;
    Expr upper_bound() override;
};

shared_ptr<InterSpace> operator&(ISpace, ISpace);
shared_ptr<UnionSpace> operator|(ISpace, ISpace);

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ITER_SPACE_ITER_H_
