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

struct PushBack : public StmtNode {
    Sym vector;
    Expr val;

    PushBack(Sym vector, Expr val) :
        StmtNode(), vector(vector), val(val)
    {
        ASSERT(vector->type.dtypes[0] == val->type);
    }

    void Accept(Visitor&) const final;
};

struct Loop : public StmtNode {
    map<Sym, Expr> idx_inits;

    // Loop increment
    map<Sym, Expr> idx_incrs;

    // Exit condition
    Expr exit_cond;

    // Body condition
    Expr body_cond;

    // Lopp body
    Stmt body;

    Loop() : StmtNode() {}

    void Accept(Visitor&) const final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_LOOP_H_
