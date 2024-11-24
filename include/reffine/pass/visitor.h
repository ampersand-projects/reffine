#ifndef INCLUDE_REFFINE_PASS_VISITOR_H_
#define INCLUDE_REFFINE_PASS_VISITOR_H_

#include "reffine/ir/expr.h"
#include "reffine/ir/stmt.h"
#include "reffine/ir/loop.h"
#include "reffine/ir/op.h"

namespace reffine {

class Visitor {
public:
    /**
     * Reffine IR
     */
    virtual void Visit(SymNode&) = 0;
    virtual void Visit(Func&) = 0;
    virtual void Visit(Call&) = 0;
    virtual void Visit(Select&) = 0;
    virtual void Visit(Const&) = 0;
    virtual void Visit(Cast&) = 0;
    virtual void Visit(Get&) = 0;
    virtual void Visit(NaryExpr&) = 0;
    virtual void Visit(Op&) = 0;
    virtual void Visit(Element&) = 0;
    virtual void Visit(Reduce&) = 0;

    /**
     * Loop IR
     */
    virtual void Visit(IsValid&) = 0;
    virtual void Visit(SetValid&) = 0;
    virtual void Visit(FetchDataPtr&) = 0;
    virtual void Visit(Stmts&) = 0;
    virtual void Visit(Alloc&) = 0;
    virtual void Visit(Load&) = 0;
    virtual void Visit(Store&) = 0;
    virtual void Visit(IfElse&) = 0;
    virtual void Visit(NoOp&) = 0;
    virtual void Visit(Loop&) = 0;

protected:
    Sym tmp_sym(SymNode& symbol)
    {
        shared_ptr<SymNode> tmp_sym(const_cast<SymNode*>(&symbol), [](SymNode*) {});
        return tmp_sym;
    }

};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_VISITOR_H_
