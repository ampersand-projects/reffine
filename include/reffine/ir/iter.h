#ifndef INCLUDE_REFFINE_IR_ITER_H_
#define INCLUDE_REFFINE_IR_ITER_H_

#include <functional>

#include "reffine/base/type.h"
#include "reffine/ir/node.h"

using namespace std;

namespace reffine {

struct Lookup : public ExprNode {
    Expr vec;
    Expr idx;

    Lookup(Expr vec, Expr idx) : ExprNode(extract_type(vec)), vec(vec), idx(idx)
    {
        ASSERT(idx->type.is_idx());
    }

    void Accept(Visitor&) final;

private:
    static DataType extract_type(Expr vec)
    {
        ASSERT(vec->type.is_vector());
        auto& vtype = vec->type;

        vector<DataType> dtypes;
        for (size_t i = vtype.dim; i < vtype.dtypes.size(); i++) {
            dtypes.push_back(vtype.dtypes[i]);
        }

        return DataType(BaseType::STRUCT, dtypes);
    }
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

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_ITER_H_
