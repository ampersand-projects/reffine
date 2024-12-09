#ifndef INCLUDE_REFFINE_IR_STMT_H
#define INCLUDE_REFFINE_IR_STMT_H

#include "reffine/ir/node.h"

namespace reffine {

struct Func : public StmtNode {
    string name;
    Expr output;
    vector<Sym> inputs;
    SymTable tbl;

    Func(string name, Expr output, vector<Sym> inputs, SymTable tbl = {})
        : StmtNode(),
          name(name),
          output(output),
          inputs(std::move(inputs)),
          tbl(std::move(tbl))
    {
    }

    void Accept(Visitor&) final;
};

struct Stmts : public StmtNode {
    vector<Stmt> stmts;

    Stmts(vector<Stmt> stmts) : StmtNode(), stmts(stmts) {}

    void Accept(Visitor&) final;
};

struct IfElse : public StmtNode {
    Expr cond;
    Stmt true_body;
    Stmt false_body;

    IfElse(Expr cond, Stmt true_body, Stmt false_body)
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
