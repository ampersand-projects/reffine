#ifndef INCLUDE_REFFINE_IR_LOOP_H_
#define INCLUDE_REFFINE_IR_LOOP_H_

#include <map>
#include <memory>
#include <vector>
#include <utility>
#include <string>

#include "reffine/ir/expr.h"

using namespace std;

namespace reffine {

struct IsValid : public ExprNode {
    Expr vec;
    Expr idx;
    size_t col;

    IsValid(Expr vec, Expr idx, size_t col) :
        ExprNode(types::BOOL), vec(vec), idx(idx), col(col)
    {
        ASSERT(vec->type.is_vector());
        ASSERT(idx->type.is_idx());
        ASSERT(col < vec->type.dtypes.size());
    }

    void Accept(Visitor&) const final;
};

struct SetValid : public ExprNode {
    Expr vec;
    Expr idx;
    Expr validity;
    size_t col;

    SetValid(Expr vec, Expr idx, Expr validity, size_t col) :
        ExprNode(types::BOOL), vec(vec), idx(idx), validity(validity), col(col)
    {
        ASSERT(vec->type.is_vector());
        ASSERT(idx->type.is_idx());
        ASSERT(validity->type == types::BOOL);
        ASSERT(col < vec->type.dtypes.size());
    }

    void Accept(Visitor&) const final;
};

struct FetchDataPtr : public ExprNode {
    Expr vec;
    Expr idx;
    size_t col;

    FetchDataPtr(Expr vec, Expr idx, size_t col) :
        ExprNode(vec->type.dtypes[col].ptr()), vec(vec), idx(idx), col(col)
    {
        ASSERT(vec->type.is_vector());
        ASSERT(idx->type.is_idx());
        ASSERT(col < vec->type.dtypes.size());
    }

    void Accept(Visitor&) const final;
};

struct Alloc : public ExprNode {
    Expr size;

    Alloc(DataType type, Expr size = make_shared<Const>(BaseType::UINT32, 1)) :
        ExprNode(type.ptr()), size(size)
    {
        ASSERT(size->type.is_int() && !size->type.is_signed());
    }

    void Accept(Visitor&) const final;
};

struct Load : public ExprNode {
    Expr addr;

    Load(Expr addr) : ExprNode(addr->type.deref()), addr(addr)
    {
        ASSERT(addr->type.is_ptr());
    }

    void Accept(Visitor&) const final;
};

struct Store : public StmtNode {
    Expr addr;
    Expr val;

    Store(Expr addr, Expr val) : StmtNode(), addr(addr), val(val)
    {
        ASSERT(addr->type.is_ptr());
        ASSERT(addr->type == val->type.ptr());
    }

    void Accept(Visitor&) const final;
};

struct Loop : public ExprNode {
    // Loop initialization
    Stmt init = nullptr;

    // Loop increment
    Stmt incr = nullptr;

    // Exit condition
    Expr exit_cond = nullptr;

    // Body condition
    Expr body_cond = nullptr;

    // Lopp body
    Stmt body = nullptr;

    // Loop post
    Stmt post = nullptr;

    // Loop output
    Expr output;

    Loop(Expr output) : ExprNode(output->type), output(output) {}

    void Accept(Visitor&) const final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_LOOP_H_
