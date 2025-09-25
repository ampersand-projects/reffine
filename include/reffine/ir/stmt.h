#ifndef INCLUDE_REFFINE_IR_STMT_H
#define INCLUDE_REFFINE_IR_STMT_H

#include "reffine/ir/node.h"

namespace reffine {

struct Func : public StmtNode {
    string name;
    Expr output;
    vector<Sym> inputs;
    SymTable tbl;
    bool is_kernel;

    Func(string name, Expr output, vector<Sym> inputs, SymTable tbl = {},
         bool is_kernel = false)
        : StmtNode(),
          name(name),
          output(output),
          inputs(std::move(inputs)),
          tbl(std::move(tbl)),
          is_kernel(is_kernel)
    {
    }

    void Accept(Visitor&) final;
};

struct Stmts : public StmtNode {
    vector<Expr> stmts;

    Stmts(vector<Expr> stmts) : StmtNode(), stmts(stmts) {}

    void Accept(Visitor&) final;
};

struct IfElse : public StmtNode {
    Expr cond;
    Expr true_body;
    Expr false_body;

    IfElse(Expr cond, Expr true_body, Expr false_body)
        : StmtNode(), cond(cond), true_body(true_body), false_body(false_body)
    {
        ASSERT(cond->type == types::BOOL);
    }

    void Accept(Visitor&) final;
};

struct NoOp : public StmtNode {
    NoOp() : StmtNode() {}

    void Accept(Visitor&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_STMT_H
