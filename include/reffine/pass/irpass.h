#ifndef INCLUDE_REFFINE_PASS_IRPASS_H_
#define INCLUDE_REFFINE_PASS_IRPASS_H_

#include <map>
#include <memory>
#include <utility>

#include "reffine/pass/visitor.h"

using namespace std;

namespace reffine {

template<typename ValTy>
class IRPassCtx {
public:
    IRPassCtx(const SymTable& in_sym_tbl) :
        in_sym_tbl(in_sym_tbl)
    {}

    const SymTable& in_sym_tbl;
    map<Sym, ValTy> sym_val_map;
    ValTy val;
};

template<typename CtxTy, typename ValTy>
class IRPass : public Visitor {
protected:
    virtual CtxTy& ctx() = 0;

    virtual ValTy visit(const Select&) = 0;
    virtual ValTy visit(const IfElse&) = 0;
    virtual ValTy visit(const Exists&) = 0;
    virtual ValTy visit(const Const&) = 0;
    virtual ValTy visit(const Cast&) = 0;
    virtual ValTy visit(const NaryExpr&) = 0;
    virtual ValTy visit(const Read&) = 0;
    virtual ValTy visit(const PushBack&) = 0;
    virtual ValTy visit(const Call&) = 0;
    virtual ValTy visit(const Stmts&) = 0;
    virtual ValTy visit(const Func&) = 0;
    virtual ValTy visit(const Loop&) = 0;

    void Visit(const Select& expr) final { val() = visit(expr); }
    void Visit(const IfElse& expr) final { val() = visit(expr); }
    void Visit(const Exists& expr) final { val() = visit(expr); }
    void Visit(const Const& expr) final { val() = visit(expr); }
    void Visit(const Cast& expr) final { val() = visit(expr); }
    void Visit(const NaryExpr& expr) final { val() = visit(expr); }
    void Visit(const Read& expr) final { val() = visit(expr); }
    void Visit(const PushBack& expr) final { val() = visit(expr); }
    void Visit(const Call& expr) final { val() = visit(expr); }
    void Visit(const Stmts& stmt) final { val() = visit(stmt); }
    void Visit(const Func& stmt) final { val() = visit(stmt); }
    void Visit(const Loop& expr) final { val() = visit(expr); }

    CtxTy& switch_ctx(CtxTy& new_ctx) { swap(new_ctx, ctx()); return new_ctx; }

    ValTy& val() { return ctx().val; }

    ValTy eval(const Expr expr)
    {
        ValTy new_val = nullptr;

        swap(new_val, val());
        expr->Accept(*this);
        swap(val(), new_val);

        return new_val;
    }

    void Visit(const SymNode& symbol) final
    {
        auto tmp = tmp_sym(symbol);

        if (ctx().sym_val_map.find(tmp) == ctx().sym_val_map.end()) {
            auto expr = ctx().in_sym_tbl.at(tmp);
            ctx().sym_val_map[tmp] = assign(tmp, expr);
        }

        val() = ctx().sym_val_map.at(tmp);
    }

    virtual ValTy assign(Sym sym, Expr expr)
    {
        return eval(expr);
    }

private:
    Sym tmp_sym(const SymNode& symbol)
    {
        shared_ptr<SymNode> tmp_sym(const_cast<SymNode*>(&symbol), [](SymNode*) {});
        return tmp_sym;
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_IRPASS_H_
