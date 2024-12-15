#ifndef INCLUDE_REFFINE_PASS_VISITOR_H_
#define INCLUDE_REFFINE_PASS_VISITOR_H_

#include "reffine/ir/expr.h"
#include "reffine/ir/loop.h"
#include "reffine/ir/op.h"
#include "reffine/ir/stmt.h"

namespace reffine {

class Visitor {
public:
    /**
     * Reffine IR
     */
    virtual void Visit(SymNode&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(Func&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(Call&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(Select&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(Const&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(Cast&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(Get&) { throw runtime_error("Operation not supported"); }
    virtual void Visit(New&) { throw runtime_error("Operation not supported"); }
    virtual void Visit(NaryExpr&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(Op&) { throw runtime_error("Operation not supported"); }
    virtual void Visit(Element&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(Reduce&)
    {
        throw runtime_error("Operation not supported");
    }

    /**
     * Loop IR
     */
    virtual void Visit(IsValid&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(SetValid&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(FetchDataPtr&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(Stmts&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(Alloc&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(Load&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(Store&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(IfElse&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(NoOp&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void Visit(Loop&)
    {
        throw runtime_error("Operation not supported");
    }

protected:
    Sym tmp_sym(SymNode& symbol)
    {
        shared_ptr<SymNode> tmp_sym(const_cast<SymNode*>(&symbol),
                                    [](SymNode*) {});
        return tmp_sym;
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_VISITOR_H_
