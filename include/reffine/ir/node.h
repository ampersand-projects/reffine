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

struct StmtNode {
    virtual ~StmtNode() {}

    virtual void Accept(Visitor&) const = 0;
};
typedef shared_ptr<StmtNode> Stmt;

struct ExprNode : public StmtNode {
    const DataType type;

    explicit ExprNode(DataType type) : StmtNode(), type(type) {}

    virtual ~ExprNode() {}
};
typedef shared_ptr<ExprNode> Expr;

struct SymNode : public ExprNode {
    const string name;

    SymNode(string name, DataType type) : ExprNode(type), name(name) {}
    SymNode(string name, Expr expr) : SymNode(name, expr->type) {}

    void Accept(Visitor&) const override;
};
typedef shared_ptr<SymNode> Sym;

typedef map<Sym, Expr> SymTable;

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_NODE_H_
