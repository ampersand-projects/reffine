#ifndef INCLUDE_REFFINE_PASS_LOOPGEN_H_
#define INCLUDE_REFFINE_PASS_LOOPGEN_H_

#include "reffine/pass/irgen.h"

namespace reffine {

class LoopGenCtx : public IRGenCtx<Sym, Expr> {
public:
    LoopGenCtx(shared_ptr<Func> old_func, shared_ptr<Func> new_func) :
        IRGenCtx(old_func->tbl, new_func->tbl)
    {}
};

class LoopGen : public IRGen<Sym, Expr> {
public:
    explicit LoopGen(LoopGenCtx& ctx) :
        IRGen(std::move(ctx))
    {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

private:
    tuple<Sym, Expr> visit(Sym, Expr) final;
    Expr visit(Call&) final;
    void visit(IfElse&) final;
    void visit(NoOp&) final;
    Expr visit(Select&) final;
    Expr visit(Exists&) final;
    Expr visit(Const&) final;
    Expr visit(Cast&) final;
    Expr visit(Get&) final;
    Expr visit(NaryExpr&) final;
    Expr visit(Op&) final { throw runtime_error("Operation not supported"); }
    Expr visit(Element&) final { throw runtime_error("Operation not supported"); }
    Expr visit(Reduce&) final { throw runtime_error("Operation not supported"); }
    void visit(Stmts&) final;
    void visit(Func&) final;
    Expr visit(Alloc&) final { throw runtime_error("Operation not supported"); }
    Expr visit(Load&) final { throw runtime_error("Operation not supported"); }
    void visit(Store&) final { throw runtime_error("Operation not supported"); }
    Expr visit(Loop&) final { throw runtime_error("Operation not supported"); }
    Expr visit(IsValid&) final { throw runtime_error("Operation not supported"); }
    Expr visit(SetValid&) final { throw runtime_error("Operation not supported"); }
    Expr visit(FetchDataPtr&) final { throw runtime_error("Operation not supported"); }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_LOOPGEN_H_
