#ifndef INCLUDE_REFFINE_PASS_BASE_IRPASS_H_
#define INCLUDE_REFFINE_PASS_BASE_IRPASS_H_

#include <memory>
#include <set>
#include <utility>

#include "reffine/pass/base/visitor.h"

using namespace std;

namespace reffine {

class IRPassCtx {
public:
    IRPassCtx(const SymTable& in_sym_tbl) : in_sym_tbl(in_sym_tbl) {}

    const SymTable& in_sym_tbl;
    set<Sym> sym_set;
};

template <typename CtxTy>
class IRPassBase : public Visitor {
public:
    explicit IRPassBase(CtxTy& ctx) : _ctx(ctx) {}

    void Visit(SymNode& symbol) override
    {
        auto tmp = tmp_sym(symbol);

        if (ctx().sym_set.find(tmp) == ctx().sym_set.end()) {
            ctx().in_sym_tbl.at(tmp)->Accept(*this);
            this->assign(tmp);
        }
    }

protected:
    CtxTy& ctx() { return _ctx; }

    void switch_ctx(CtxTy& new_ctx) { swap(new_ctx, ctx()); }
    void assign(Sym sym) { ctx().sym_set.insert(sym); }

    Sym tmp_sym(SymNode& symbol)
    {
        shared_ptr<SymNode> tmp_sym(const_cast<SymNode*>(&symbol),
                                    [](SymNode*) {});
        return tmp_sym;
    }

private:
    CtxTy& _ctx;
};

template <typename CtxTy>
class IRPass : public IRPassBase<CtxTy> {
public:
    explicit IRPass(CtxTy& ctx) : IRPassBase<CtxTy>(ctx) {}

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
        for (auto& idx : expr.idxs) { this->assign(idx); }
        for (auto& pred : expr.preds) { pred->Accept(*this); }
        for (auto& output : expr.outputs) { output->Accept(*this); }
    }

    void Visit(Element& expr) override
    {
        expr.vec->Accept(*this);
        for (auto& idx : expr.idxs) { idx->Accept(*this); }
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

    void Visit(FetchDataPtr& expr) override
    {
        expr.vec->Accept(*this);
        expr.idx->Accept(*this);
    }

    void Visit(NoOp& stmt) override {}

protected:
    void Visit(SymNode& sym) final { IRPassBase<CtxTy>::Visit(sym); }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_BASE_IRPASS_H_
