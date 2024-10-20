#ifndef INCLUDE_REFFINE_PASS_VISITOR_H_
#define INCLUDE_REFFINE_PASS_VISITOR_H_

#include "reffine/ir/expr.h"
#include "reffine/ir/stmt.h"
#include "reffine/ir/loop.h"

namespace reffine {

class Visitor {
public:
    /**
     * Reffine IR
     */
    virtual void Visit(const SymNode&) = 0;
    virtual void Visit(const Func&) = 0;
    virtual void Visit(const Call&) = 0;
    virtual void Visit(const Select&) = 0;
    virtual void Visit(const Exists&) = 0;
    virtual void Visit(const Const&) = 0;
    virtual void Visit(const Cast&) = 0;
    virtual void Visit(const NaryExpr&) = 0;
    virtual void Visit(const Read&) = 0;
    virtual void Visit(const Write&) = 0;

    /**
     * Loop IR
     */
    virtual void Visit(const IsValid&) = 0;
    virtual void Visit(const SetValid&) = 0;
    virtual void Visit(const FetchDataPtr&) = 0;
    virtual void Visit(const Stmts&) = 0;
    virtual void Visit(const Alloc&) = 0;
    virtual void Visit(const Load&) = 0;
    virtual void Visit(const Store&) = 0;
    virtual void Visit(const IfElse&) = 0;
    virtual void Visit(const NoOp&) = 0;
    virtual void Visit(const Loop&) = 0;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_VISITOR_H_
