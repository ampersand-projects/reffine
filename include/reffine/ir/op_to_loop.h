#ifndef INCLUDE_REFFINE_IR_OP_TO_LOOP_H_
#define INCLUDE_REFFINE_IR_OP_TO_LOOP_H_

#include "reffine/base/type.h"
#include "reffine/ir/node.h"

using namespace std;

namespace reffine {

struct SubVector : public ExprNode {
    Expr vec;
    Expr start;
    Expr end;

    SubVector(Expr vec, Expr start, Expr end)
        : ExprNode(vec->type.valty()), vec(vec), start(start), end(end)
    {
        ASSERT(vec->type.is_vector());
        ASSERT(start->type.is_idx());
        ASSERT(end->type.is_idx());
    }

    void Accept(Visitor&) final;
};

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

struct ReadRunEnd : public ExprNode {
    Expr vec;
    Expr idx;
    size_t col;

    ReadRunEnd(Expr vec, Expr idx, size_t col)
        : ExprNode(types::IDX), vec(vec), idx(idx), col(col)
    {
        ASSERT(this->vec->type.is_vector());
        ASSERT(this->col < this->vec->type.dtypes.size());
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
        ASSERT(vtype.dim <= 2);
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
    IsValid(Expr buf, Expr idx)
        : Call("get_null_bit", types::BOOL, vector<Expr>{buf, idx})
    {
        ASSERT(buf->type == types::UINT16.ptr());
        ASSERT(idx->type.is_idx());
    }
};

struct SetValid : public Call {
    SetValid(Expr buf, Expr idx, Expr validity)
        : Call("set_null_bit", types::VOID, vector<Expr>{buf, idx, validity})
    {
        ASSERT(buf->type == types::UINT16.ptr());
        ASSERT(idx->type.is_idx());
        ASSERT(validity->type == types::BOOL);
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

struct BuildIndex : public Call {
    BuildIndex(Expr vec) : Call("build_vector_index", vec->type, vector<Expr>{vec})
    {
        ASSERT(vec->type.is_vector());
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
        : Call("get_array_child", types::VOID.ptr(),
               vector<Expr>{arr, make_shared<Const>(types::UINT32, col)})
    {
        ASSERT(arr->type == types::VOID.ptr());
    }
};

struct GetArrayBuf : public Call {
    GetArrayBuf(Expr arr, size_t col)
        : Call("get_array_buf", types::VOID.ptr(),
               vector<Expr>{arr, make_shared<Const>(types::UINT32, col)})
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

struct ReadRunEndBuf : public Call {
    ReadRunEndBuf(Expr buf, Expr idx)
        : Call("read_runend_buf", types::IDX, vector<Expr>{buf, idx})
    {
        ASSERT(buf->type == types::INT32.ptr());
        ASSERT(idx->type.is_idx());
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_OP_TO_LOOP_H_
