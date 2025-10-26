#ifndef INCLUDE_REFFINE_IR_OP_TO_LOOP_H_
#define INCLUDE_REFFINE_IR_OP_TO_LOOP_H_

#include "reffine/base/type.h"
#include "reffine/ir/node.h"

using namespace std;

namespace reffine {

struct ReadData : public ExprNode {
    Expr vec;
    Expr idx;
    size_t col;

    ReadData(Expr vec, Expr idx, size_t col)
        : ExprNode(vec->type.dtypes[col]), vec(vec), idx(idx), col(col)
    {
        ASSERT(this->vec->type.is_vector());
        ASSERT(this->col < this->vec->type.dtypes.size());
    }

    void Accept(Visitor&) final;
};

struct WriteData : public StmtNode {
    Expr vec;
    Expr idx;
    size_t col;
    Expr val;

    WriteData(Expr vec, Expr idx, size_t col, Expr val)
        : StmtNode(), vec(vec), idx(idx), col(col), val(val)
    {
        ASSERT(this->vec->type.is_vector());
        ASSERT(this->col < this->vec->type.dtypes.size());
        ASSERT(this->val->type == this->vec->type.dtypes[col]);
    }

    void Accept(Visitor&) final;
};

struct ReadBit : public ExprNode {
    Expr vec;
    Expr idx;
    size_t col;

    ReadBit(Expr vec, Expr idx, size_t col)
        : ExprNode(types::BOOL), vec(vec), idx(idx), col(col)
    {
        ASSERT(this->vec->type.is_vector());
        ASSERT(this->col < this->vec->type.dtypes.size());
    }

    void Accept(Visitor&) final;
};

struct WriteBit : public StmtNode {
    Expr vec;
    Expr idx;
    size_t col;
    Expr val;

    WriteBit(Expr vec, Expr idx, size_t col, Expr val)
        : StmtNode(), vec(vec), idx(idx), col(col), val(val)
    {
        ASSERT(this->vec->type.is_vector());
        ASSERT(this->col < this->vec->type.dtypes.size());
        ASSERT(this->val->type == types::BOOL);
    }

    void Accept(Visitor&) final;
};

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

struct Length : public ExprNode {
    Expr vec;
    size_t col;

    Length(Expr vec, size_t col) : ExprNode(types::IDX), vec(vec), col(col)
    {
        ASSERT(vec->type.is_vector());
        ASSERT(col < vec->type.dtypes.size());
    }

    void Accept(Visitor&) final;
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
        : Call("set_vector_null_bit", types::VOID,
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

struct FinalizeVector : public Call {
    FinalizeVector(Expr vec, Expr bytemap, Expr len, Expr null_count)
        : Call("finalize_vector", types::VOID,
               vector<Expr>{vec, bytemap, len, null_count})
    {
        ASSERT(vec->type.is_vector());
        ASSERT(bytemap->type.deref() == types::BOOL);
        ASSERT(len->type.is_idx());
        ASSERT(null_count->type.is_idx());
    }
};

struct GetVectorArray : public Call {
    GetVectorArray(Expr vec)
        : Call("get_vector_array", types::VOID.ptr(), vector<Expr>{vec})
    {
        ASSERT(vec->type.is_vector());
    }
};

struct GetArrayChild : public Call {
    GetArrayChild(Expr arr, size_t col)
        : Call("get_array_child", types::VOID.ptr(), vector<Expr>{arr, make_shared<Const>(types::UINT32, col)})
    {
        ASSERT(arr->type == types::VOID.ptr());
    }
};

struct GetArrayBuf : public Call {
    GetArrayBuf(Expr arr, size_t col)
        : Call("get_array_buf", types::VOID.ptr(), vector<Expr>{arr, make_shared<Const>(types::UINT32, col)})
    {
        ASSERT(arr->type == types::VOID.ptr());
    }
};

struct GetArrayLength : public Call {
    GetArrayLength(Expr arr)
        : Call("get_array_len", types::IDX, vector<Expr>{arr})
    {
        ASSERT(arr->type == types::VOID.ptr());
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_OP_TO_LOOP_H_
