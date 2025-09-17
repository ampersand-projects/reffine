#ifndef INCLUDE_REFFINE_IR_OP_TO_LOOP_H_
#define INCLUDE_REFFINE_IR_OP_TO_LOOP_H_

#include "reffine/base/type.h"
#include "reffine/ir/node.h"

using namespace std;

namespace reffine {

struct Locate : public Call {
    Expr vec;
    Expr iter;

    Locate(Expr vec, Expr iter)
        : Call("vector_locate", types::IDX, vector<Expr>{vec, iter})
    {
        auto& vtype = vec->type;

        ASSERT(vtype.is_vector());
        ASSERT(vtype.dim == 1);
        ASSERT(vtype.iterty() == iter->type);
    }
};

struct Length : public Call {
    Length(Expr vec) : Call("get_vector_len", types::IDX, vector<Expr>{vec})
    {
        ASSERT(vec->type.is_vector());
    }
};

struct SetLength : public Call {
    SetLength(Expr vec, Expr idx)
        : Call("set_vector_len", types::IDX, vector<Expr>{vec, idx})
    {
        ASSERT(vec->type.is_vector());
        ASSERT(idx->type.is_idx());
    }
};

struct IsValid : public Call {
    IsValid(Expr vec, Expr idx, size_t col)
        : Call("get_vector_null_bit", types::BOOL,
               vector<Expr>{vec, idx, make_shared<Const>(types::UINT32, col)})
    {
        ASSERT(vec->type.is_vector());
        ASSERT(idx->type.is_idx());
        ASSERT(col < vec->type.dtypes.size());
    }
};

struct SetValid : public Call {
    SetValid(Expr vec, Expr idx, Expr validity, size_t col)
        : Call("set_vector_null_bit", types::BOOL,
               vector<Expr>{vec, idx, validity,
                            make_shared<Const>(types::UINT32, col)})
    {
        ASSERT(vec->type.is_vector());
        ASSERT(idx->type.is_idx());
        ASSERT(validity->type == types::BOOL);
        ASSERT(col < vec->type.dtypes.size());
    }
};

struct MakeVector : public Call {
    MakeVector(DataType type, Expr len, size_t mem_id)
        : Call("make_vector", type,
               vector<Expr>{len, make_shared<Const>(types::UINT32, mem_id)})
    {
        ASSERT(type.is_vector());
        ASSERT(len->type == types::IDX);
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_OP_TO_LOOP_H_
