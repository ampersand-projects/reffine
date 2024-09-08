#ifndef INCLUDE_REFFINE_PASS_VISITOR_H_
#define INCLUDE_REFFINE_PASS_VISITOR_H_

#include "reffine/ir/expr.h"
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
    virtual void Visit(const IfElse&) = 0;
    virtual void Visit(const Exists&) = 0;
    virtual void Visit(const Const&) = 0;
    virtual void Visit(const Cast&) = 0;
    virtual void Visit(const NaryExpr&) = 0;

    /**
     * Loop IR
     */
    virtual void Visit(const Stmts&) = 0;
    virtual void Visit(const Read&) = 0;
    virtual void Visit(const PushBack&) = 0;
    virtual void Visit(const Loop&) = 0;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_VISITOR_H_
