#ifndef INCLUDE_REFFINE_IR_OP_TO_LOOP_H_
#define INCLUDE_REFFINE_IR_OP_TO_LOOP_H_

#include "reffine/base/type.h"
#include "reffine/ir/node.h"

using namespace std;

namespace reffine {

struct Lookup : public ExprNode {
    Expr vec;
    Expr idx;

    Lookup(Expr vec, Expr idx)
        : ExprNode(vec->type.iterty()), vec(vec), idx(idx)
    {
        ASSERT(vec->type.is_vector());
        ASSERT(idx->type.is_idx());
    }

    void Accept(Visitor&) final;
};

struct Locate : public ExprNode {
    Expr vec;
    Expr iter;

    Locate(Expr vec, Expr iter) : ExprNode(types::IDX), vec(vec), iter(iter)
    {
        auto& vtype = vec->type;

        ASSERT(vtype.is_vector());
        ASSERT(vtype.dim == 1);
        ASSERT(vtype.iterty() == iter->type);
    }

    void Accept(Visitor&) final;
};

struct Length : public ExprNode {
    Expr vec;

    Length(Expr vec) : ExprNode(types::IDX), vec(vec)
    {
        ASSERT(vec->type.is_vector());
    }

    void Accept(Visitor&) final;
};

struct IsValid : public ExprNode {
    Expr vec;
    Expr idx;
    size_t col;

    IsValid(Expr vec, Expr idx, size_t col)
        : ExprNode(types::BOOL), vec(vec), idx(idx), col(col)
    {
        ASSERT(vec->type.is_vector());
        ASSERT(idx->type.is_idx());
        ASSERT(col < vec->type.dtypes.size());
    }

    void Accept(Visitor&) final;
};

struct SetValid : public ExprNode {
    Expr vec;
    Expr idx;
    Expr validity;
    size_t col;

    SetValid(Expr vec, Expr idx, Expr validity, size_t col)
        : ExprNode(types::BOOL),
          vec(vec),
          idx(idx),
          validity(validity),
          col(col)
    {
        ASSERT(vec->type.is_vector());
        ASSERT(idx->type.is_idx());
        ASSERT(validity->type == types::BOOL);
        ASSERT(col < vec->type.dtypes.size());
    }

    void Accept(Visitor&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_OP_TO_LOOP_H_
