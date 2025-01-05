#ifndef INCLUDE_REFFINE_PASS_BASE_VISITOR_H_
#define INCLUDE_REFFINE_PASS_BASE_VISITOR_H_

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
        throw runtime_error("SymNode operation not supported");
    }
    virtual void Visit(Func&)
    {
        throw runtime_error("Func operation not supported");
    }
    virtual void Visit(Call&)
    {
        throw runtime_error("Call operation not supported");
    }
    virtual void Visit(Select&)
    {
        throw runtime_error("Select operation not supported");
    }
    virtual void Visit(Const&)
    {
        throw runtime_error("Const operation not supported");
    }
    virtual void Visit(Cast&)
    {
        throw runtime_error("Cast operation not supported");
    }
    virtual void Visit(Get&)
    {
        throw runtime_error("Get operation not supported");
    }
    virtual void Visit(New&)
    {
        throw runtime_error("New operation not supported");
    }
    virtual void Visit(NaryExpr&)
    {
        throw runtime_error("NaryExpr operation not supported");
    }
    virtual void Visit(Op&)
    {
        throw runtime_error("Op operation not supported");
    }
    virtual void Visit(Element&)
    {
        throw runtime_error("Element operation not supported");
    }
    virtual void Visit(NotNull&)
    {
        throw runtime_error("NotNull operation not supported");
    }
    virtual void Visit(Reduce&)
    {
        throw runtime_error("Reduce operation not supported");
    }

    /**
     * Loop IR
     */
    virtual void Visit(IsValid&)
    {
        throw runtime_error("IsValid operation not supported");
    }
    virtual void Visit(SetValid&)
    {
        throw runtime_error("SetValid operation not supported");
    }
    virtual void Visit(Lookup&)
    {
        throw runtime_error("Lookup operation not supported");
    }
    virtual void Visit(Locate&)
    {
        throw runtime_error("Locate operation not supported");
    }
    virtual void Visit(FetchDataPtr&)
    {
        throw runtime_error("FetchDataPtr operation not supported");
    }
    virtual void Visit(Stmts&)
    {
        throw runtime_error("Stmts operation not supported");
    }
    virtual void Visit(Alloc&)
    {
        throw runtime_error("Alloc operation not supported");
    }
    virtual void Visit(Load&)
    {
        throw runtime_error("Load operation not supported");
    }
    virtual void Visit(Store&)
    {
        throw runtime_error("Store operation not supported");
    }
    virtual void Visit(IfElse&)
    {
        throw runtime_error("IfElse operation not supported");
    }
    virtual void Visit(NoOp&)
    {
        throw runtime_error("NoOp operation not supported");
    }
    virtual void Visit(Loop&)
    {
        throw runtime_error("Loop operation not supported");
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_BASE_VISITOR_H_
