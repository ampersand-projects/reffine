#ifndef INCLUDE_REFFINE_IR_NODE_H_
#define INCLUDE_REFFINE_IR_NODE_H_

#include <memory>
#include <map>
#include <vector>
#include <utility>
#include <string>

#include "reffine/base/type.h"

using namespace std;

namespace reffine {

class Visitor;
struct ExprNode;
typedef shared_ptr<ExprNode> Expr;
struct SymNode;
typedef shared_ptr<SymNode> Sym;
typedef vector<Sym> Params;
typedef map<Sym, Expr> SymTable;
typedef map<Sym, Sym> Aux;

struct StmtNode {
    virtual ~StmtNode() {}

    virtual void Accept(Visitor&) const = 0;
};
typedef shared_ptr<StmtNode> Stmt;

struct ExprNode : public StmtNode {
    const DataType type;

    explicit ExprNode(DataType type) : StmtNode(), type(type) {}

    virtual ~ExprNode() {}

    virtual void Accept(Visitor&) const = 0;
};

struct SymNode : public ExprNode {
    const string name;

    SymNode(string name, DataType type) : ExprNode(type), name(name) {}
    SymNode(string name, Expr expr) : SymNode(name, expr->type) {}

    void Accept(Visitor&) const override;
};

struct FuncNode : public ExprNode {
    string name;
    Params inputs;
    Sym output;
    SymTable syms;

    FuncNode(string name, Params inputs, Sym output, SymTable syms) :
        ExprNode(output->type), name(name), inputs(std::move(inputs)), output(output), syms(std::move(syms))
    {}

protected:
    FuncNode(string name, DataType type) : ExprNode(std::move(type)), name(name) {}
};
typedef shared_ptr<FuncNode> Func;

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_NODE_H_
