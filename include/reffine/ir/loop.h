#ifndef INCLUDE_REFFINE_IR_LOOP_H_
#define INCLUDE_REFFINE_IR_LOOP_H_

#include <map>
#include <memory>
#include <vector>
#include <utility>
#include <string>

#include "reffine/ir/node.h"

using namespace std;

namespace reffine {

struct Read : public ExprNode {
    Sym vector;
    Sym idx;

    Read(Sym vector, Sym idx) :
        ExprNode(vector->type.dtypes[0]), vector(vector), idx(idx)
    {
        ASSERT(vector->type.is_vector());
        ASSERT(idx->type.is_idx());
    }

    void Accept(Visitor&) const final;
};

struct PushBack : public ExprNode {
    Expr vector;
    Expr val;

    PushBack(Expr vector, Expr val) :
        ExprNode(vector->type), vector(vector), val(val)
    {
        ASSERT(val->type.is_val());
        ASSERT(vector->type.dtypes[0] == val->type);
        ASSERT(vector->type.is_vector());
    }

    void Accept(Visitor&) const final;
};

struct Stmts : public StmtNode {
    vector<Stmt> stmts;

    Stmts(vector<Stmt> stmts) : StmtNode(), stmts(stmts) {}

    void Accept(Visitor&) const final;
};

struct Assign : public StmtNode {
    Sym lhs;
    Expr rhs;

    Assign(Sym lhs, Expr rhs) : StmtNode(), lhs(lhs), rhs(rhs) {}

    void Accept(Visitor&) const final;
};

struct Loop : public ExprNode {
    // Loop initialization
    Stmt init;

    // Loop increment
    Stmt incr;

    // Exit condition
    Expr exit_cond;

    // Body condition
    Expr body_cond;

    // Lopp body
    Stmt body;

    // Loop output
    Expr output;

    Loop(Expr output) : ExprNode(output->type), output(output) {}

    void Accept(Visitor&) const final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_LOOP_H_
