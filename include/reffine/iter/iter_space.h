#ifndef INCLUDE_REFFINE_ITER_SPACE_ITER_H_
#define INCLUDE_REFFINE_ITER_SPACE_ITER_H_

#include "reffine/base/type.h"
#include "reffine/ir/node.h"

using namespace std;

namespace reffine {

struct IterSpace {
    const DataType type;

    IterSpace(DataType type) : type(type)
    {
        ASSERT(type.is_val());
    }

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
    Expr bound;

    BoundSpace(Expr bound) : IterSpace(bound->type), bound(bound) {}
};

struct LBoundSpace : public BoundSpace {
    LBoundSpace(Expr bound) : BoundSpace(bound) {}

    Expr lower_bound() final;
};

struct UBoundSpace : public BoundSpace {
    UBoundSpace(Expr bound) : BoundSpace(bound) {}

    Expr upper_bound() final;
};

struct JointSpace : public IterSpace {
    ISpace left;
    ISpace right;

    JointSpace(ISpace left, ISpace right) : IterSpace(left->type), left(left), right(right)
    {
        ASSERT(left->type == right->type);
    }
};

struct UnionSpace : public JointSpace {
    UnionSpace(ISpace left, ISpace right) : JointSpace(left, right) {}

    Expr lower_bound() override;
    Expr upper_bound() override;
};

struct InterSpace : public JointSpace {
    InterSpace(ISpace left, ISpace right) : JointSpace(left, right) {}

    Expr lower_bound() override;
    Expr upper_bound() override;
};

ISpace operator&(ISpace, ISpace);
ISpace operator|(ISpace, ISpace);
ISpace operator>=(ISpace, Expr);
ISpace operator<=(ISpace, Expr);

}  // namespace reffine

#endif  // INCLUDE_REFFINE_ITER_SPACE_ITER_H_
