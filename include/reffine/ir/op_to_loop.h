#ifndef INCLUDE_REFFINE_IR_OP_TO_LOOP_H_
#define INCLUDE_REFFINE_IR_OP_TO_LOOP_H_

#include "reffine/base/type.h"
#include "reffine/ir/node.h"

using namespace std;

namespace reffine {

struct Lookup : public ExprNode {
    Expr vec;
    Expr idx;

    Lookup(Expr vec, Expr idx) : ExprNode(vec->type.iterty()), vec(vec), idx(idx)
    {
        ASSERT(vec->type.is_vector());
        ASSERT(idx->type.is_idx());
    }

    void Accept(Visitor&) final;
};

struct Locate : public ExprNode {
    Expr vec;
    vector<Expr> iters;

    Locate(Expr vec, vector<Expr> iters)
        : ExprNode(types::IDX), vec(vec), iters(iters)
    {
        auto& vtype = vec->type;

        ASSERT(vtype.is_vector());
        ASSERT(vtype.dim >= this->iters.size());
        for (size_t i = 0; i < this->iters.size(); i++) {
            ASSERT(vtype.dtypes[i] == this->iters[i]->type);
        }
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

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_OP_TO_LOOP_H_
