#ifndef INCLUDE_REFFINE_PASS_BASE_IRPASS_H_
#define INCLUDE_REFFINE_PASS_BASE_IRPASS_H_

#include <memory>
#include <set>
#include <utility>

#include "reffine/pass/base/visitor.h"

using namespace std;

namespace reffine {

template <typename ValTy>
class IRPassBaseCtx {
public:
    IRPassBaseCtx(const SymTable& in_sym_tbl, map<Sym, ValTy>& out_sym_tbl)
        : in_sym_tbl(in_sym_tbl), out_sym_tbl(out_sym_tbl)
    {
    }

    const SymTable& in_sym_tbl;
    map<Sym, ValTy>& out_sym_tbl;
};

template <typename ValTy>
class IRPassBase : public Visitor {
public:
    explicit IRPassBase(IRPassBaseCtx<ValTy>& ctx) : _ctx(ctx) {}

protected:
    IRPassBaseCtx<ValTy>& ctx() { return _ctx; }

    void assign(Sym sym, ValTy val) { ctx().out_sym_tbl[sym] = val; }

    Sym tmp_sym(SymNode& symbol)
    {
        shared_ptr<SymNode> tmp_sym(const_cast<SymNode*>(&symbol),
                                    [](SymNode*) {});
        return tmp_sym;
    }

    Expr tmp_expr(ExprNode& expr)
    {
        Expr tmp_expr(const_cast<ExprNode*>(&expr), [](ExprNode*) {});
        return tmp_expr;
    }

private:
    IRPassBaseCtx<ValTy>& _ctx;
};

class IRPassCtx : public IRPassBaseCtx<Sym> {
public:
    IRPassCtx(const SymTable& in_sym_tbl, map<Sym, Sym> m = {})
        : IRPassBaseCtx<Sym>(in_sym_tbl, m)
    {
    }
};

class IRPass : public IRPassBase<Sym> {
public:
    explicit IRPass(IRPassCtx& ctx) : IRPassBase(ctx) {}

    void Visit(Select& expr) override
    {
        expr.cond->Accept(*this);
        expr.true_body->Accept(*this);
        expr.false_body->Accept(*this);
    }

    void Visit(IfElse& stmt) override
    {
        stmt.cond->Accept(*this);
        stmt.true_body->Accept(*this);
        stmt.false_body->Accept(*this);
    }

    void Visit(Const& expr) override {}

    void Visit(Cast& expr) override { expr.arg->Accept(*this); }

    void Visit(Get& expr) override { expr.val->Accept(*this); }

    void Visit(New& expr) override
    {
        for (auto& val : expr.vals) { val->Accept(*this); }
    }

    void Visit(NaryExpr& expr) override
    {
        for (auto& arg : expr.args) { arg->Accept(*this); }
    }

    void Visit(Op& expr) override
    {
        for (auto& iter : expr.iters) { this->assign(iter); }
        expr.pred->Accept(*this);
        for (auto& output : expr.outputs) { output->Accept(*this); }
    }

    void Visit(Element& expr) override
    {
        expr.vec->Accept(*this);
        for (auto& iter : expr.iters) { iter->Accept(*this); }
    }

    void Visit(NotNull& expr) override { expr.elem->Accept(*this); }

    void Visit(Reduce& expr) override { expr.op.Accept(*this); }

    void Visit(Call& expr) override
    {
        for (auto& arg : expr.args) { arg->Accept(*this); }
    }

    void Visit(Stmts& stmt) override
    {
        for (auto& stmt : stmt.stmts) { stmt->Accept(*this); }
    }

    void Visit(Func& stmt) override
    {
        for (auto& input : stmt.inputs) { this->assign(input); }

        stmt.output->Accept(*this);
    }

    void Visit(Alloc& expr) override { expr.size->Accept(*this); }

    void Visit(Load& expr) override { expr.addr->Accept(*this); }

    void Visit(Store& expr) override
    {
        expr.addr->Accept(*this);
        expr.val->Accept(*this);
    }

    void Visit(Loop& expr) override
    {
        if (expr.init) { expr.init->Accept(*this); }
        if (expr.incr) { expr.incr->Accept(*this); }
        expr.exit_cond->Accept(*this);
        if (expr.body_cond) { expr.body_cond->Accept(*this); }
        expr.body->Accept(*this);
        if (expr.post) { expr.post->Accept(*this); }
        expr.output->Accept(*this);
    }

    void Visit(IsValid& expr) override
    {
        expr.vec->Accept(*this);
        expr.idx->Accept(*this);
    }

    void Visit(SetValid& expr) override
    {
        expr.vec->Accept(*this);
        expr.idx->Accept(*this);
        expr.validity->Accept(*this);
    }

    void Visit(Lookup& expr) override
    {
        expr.vec->Accept(*this);
        expr.idx->Accept(*this);
    }

    void Visit(Locate& expr) override
    {
        expr.vec->Accept(*this);
        for (auto& iter : expr.iters) { iter->Accept(*this); }
    }

    void Visit(FetchDataPtr& expr) override
    {
        expr.vec->Accept(*this);
        expr.idx->Accept(*this);
    }

    void Visit(NoOp& stmt) override {}

protected:
    void Visit(SymNode& symbol) override
    {
        auto tmp = tmp_sym(symbol);

        if (ctx().out_sym_tbl.find(tmp) == ctx().out_sym_tbl.end()) {
            ctx().in_sym_tbl.at(tmp)->Accept(*this);
            this->assign(tmp);
        }
    }

    void assign(Sym sym) { IRPassBase<Sym>::assign(sym, sym); }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_BASE_IRPASS_H_
