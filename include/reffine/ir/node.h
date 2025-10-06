#ifndef INCLUDE_REFFINE_IR_NODE_H_
#define INCLUDE_REFFINE_IR_NODE_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "reffine/base/type.h"

using namespace std;

namespace reffine {

class Visitor;
struct SymNode;
typedef shared_ptr<SymNode> Sym;

struct ExprNode {
    const DataType type;

    explicit ExprNode(DataType type) : type(type) {}

    virtual ~ExprNode() {}

    virtual void Accept(Visitor&) = 0;

    string str();

    Sym symify();
};
typedef shared_ptr<ExprNode> Expr;

struct StmtNode : public ExprNode {
    explicit StmtNode() : ExprNode(types::VOID) {}

    virtual ~StmtNode() {}
};
typedef shared_ptr<StmtNode> Stmt;

struct SymNode : public ExprNode {
    const string name;

    SymNode(string name, DataType type) : ExprNode(type), name(name) {}
    SymNode(string name, Expr expr) : SymNode(name, expr->type) {}

    void Accept(Visitor&) final;
};

typedef map<Sym, Expr> SymTable;

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_NODE_H_
