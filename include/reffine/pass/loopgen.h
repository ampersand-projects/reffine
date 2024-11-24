#ifndef INCLUDE_REFFINE_PASS_LOOPGEN_H_
#define INCLUDE_REFFINE_PASS_LOOPGEN_H_

#include "reffine/pass/irgen.h"

namespace reffine {

class LoopGenCtx : public IRGenCtx<Sym, Expr> {
public:
    LoopGenCtx(shared_ptr<Func> op_func, shared_ptr<Func> loop_func) :
        IRGenCtx(op_func->tbl, loop_func->tbl), loop_func(loop_func)
    {}

private:
    shared_ptr<Func> loop_func;

    friend class LoopGen;
};

class LoopGen : public IRGen<Sym, Expr> {
public:
    explicit LoopGen(LoopGenCtx& ctx) :
        IRGen(ctx), _ctx(ctx)
    {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

private:
    tuple<Sym, Expr> visit(Sym, Expr) final;
    Expr visit(Call&) final;
    Expr visit(Select&) final;
    Expr visit(Const&) final;
    Expr visit(Cast&) final;
    Expr visit(Get&) final;
    Expr visit(NaryExpr&) final;
    Expr visit(Op&) final;
    Expr visit(Element&) final;
    Expr visit(Reduce&) final;
    void visit(Func&) final;

    void visit(Stmts&) final { throw runtime_error("Operation not supported"); }
    void visit(IfElse&) final { throw runtime_error("Operation not supported"); }
    void visit(NoOp&) final { throw runtime_error("Operation not supported"); }
    Expr visit(Alloc&) final { throw runtime_error("Operation not supported"); }
    Expr visit(Load&) final { throw runtime_error("Operation not supported"); }
    void visit(Store&) final { throw runtime_error("Operation not supported"); }
    Expr visit(Loop&) final { throw runtime_error("Operation not supported"); }
    Expr visit(IsValid&) final { throw runtime_error("Operation not supported"); }
    Expr visit(SetValid&) final { throw runtime_error("Operation not supported"); }
    Expr visit(FetchDataPtr&) final { throw runtime_error("Operation not supported"); }

    LoopGenCtx& _ctx;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_LOOPGEN_H_
