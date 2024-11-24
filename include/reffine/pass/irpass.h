#ifndef INCLUDE_REFFINE_PASS_IRPASS_H_
#define INCLUDE_REFFINE_PASS_IRPASS_H_

#include <set>
#include <memory>
#include <utility>

#include "reffine/pass/visitor.h"

using namespace std;

namespace reffine {

class IRPassCtx {
public:
    IRPassCtx(SymTable& in_sym_tbl) :
        in_sym_tbl(in_sym_tbl)
    {}

    SymTable& in_sym_tbl;
    set<Sym> sym_set;
};

template<typename CtxTy>
class IRPass : public Visitor {
public:
    explicit IRPass(CtxTy ctx) : _ctx(std::move(ctx)) {}

    void Visit(Select& expr) override
    {
        eval(expr.cond);
        eval(expr.true_body);
        eval(expr.false_body);
    }

    void Visit(IfElse& stmt) override
    {
        eval(stmt.cond);
        eval(stmt.true_body);
        eval(stmt.false_body);
    }

    void Visit(Const& expr) override {}

    void Visit(Cast& expr) override { eval(expr.arg); }

    void Visit(Get& expr) override { eval(expr.val); }

    void Visit(NaryExpr& expr) override
    {
        for (auto& arg : expr.args) {
            eval(arg);
        }
    }

    void Visit(Op& expr) override
    {
        for (auto& pred : expr.preds) {
            eval(pred);
        }
        for (auto& output : expr.outputs) {
            eval(output);
        }
    }

    void Visit(Element& expr) override
    {
        eval(expr.vec);
        for (auto& idx : expr.idxs) {
            eval(idx);
        }
    }

    void Visit(Reduce& expr) override
    {
        eval(expr.vec);
    }

    void Visit(Call& expr) override
    {
        for (auto& arg : expr.args) {
            eval(arg);
        }
    }

    void Visit(Stmts& stmt) override
    {
        for (auto& stmt : stmt.stmts) {
            eval(stmt);
        }
    }

    void Visit(Func& stmt) override
    {
        for (auto& input : stmt.inputs) {
            assign(input);
        }

        eval(stmt.output);
    }

    void Visit(Alloc& expr) override { eval(expr.size); }

    void Visit(Load& expr) override { eval(expr.addr); }

    void Visit(Store& expr) override
    {
        eval(expr.addr);
        eval(expr.val);
    }

    void Visit(Loop& expr) override
    {
        if (expr.init) { eval(expr.init); }
        if (expr.incr) { eval(expr.incr); }
        eval(expr.exit_cond);
        if (expr.body_cond) { eval(expr.body_cond); }
        eval(expr.body);
        if (expr.post) { eval(expr.post); }
        eval(expr.output);
    }

    void Visit(IsValid& expr) override
    {
        eval(expr.vec);
        eval(expr.idx);
    }

    void Visit(SetValid& expr) override
    {
        eval(expr.vec);
        eval(expr.idx);
        eval(expr.validity);
    }

    void Visit(FetchDataPtr& expr) override
    {
        eval(expr.vec);
        eval(expr.idx);
    }

    void Visit(NoOp& stmt) override {}

protected:
    virtual CtxTy& ctx() { return _ctx; }

    CtxTy& switch_ctx(CtxTy& new_ctx) { swap(new_ctx, ctx()); return new_ctx; }

    void eval(Stmt stmt) { stmt->Accept(*this); }

    void Visit(SymNode& symbol) final
    {
        auto tmp = tmp_sym(symbol);

        if (ctx().sym_set.find(tmp) == ctx().sym_set.end()) {
            eval(ctx().in_sym_tbl.at(tmp));
            assign(tmp);
        }
    }

    virtual void assign(Sym sym)
    {
        ctx().sym_set.insert(sym);
    }

private:
    CtxTy _ctx;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_IRPASS_H_
