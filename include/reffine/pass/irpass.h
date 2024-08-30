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
    IRPassCtx(const map<Sym, Expr>& in_sym_tbl) :
        in_sym_tbl(in_sym_tbl)
    {}

    const map<Sym, Expr>& in_sym_tbl;
    map<Sym, ValTy> sym_val_map;
    ValTy val;
};

template<typename CtxTy, typename ValTy>
class IRPass : public Visitor {
protected:
    virtual CtxTy& ctx() = 0;

    virtual ValTy visit(const SymNode&) = 0;
    virtual ValTy visit(const Select&) = 0;
    virtual ValTy visit(const Exists&) = 0;
    virtual ValTy visit(const Const&) = 0;
    virtual ValTy visit(const Cast&) = 0;
    virtual ValTy visit(const NaryExpr&) = 0;
    virtual ValTy visit(const Read&) = 0;
    virtual ValTy visit(const PushBack&) = 0;
    virtual ValTy visit(const Call&) = 0;
    virtual ValTy visit(const LoopNode&) = 0;

    void Visit(const Select& expr) final { val() = visit(expr); }
    void Visit(const Exists& expr) final { val() = visit(expr); }
    void Visit(const Const& expr) final { val() = visit(expr); }
    void Visit(const Cast& expr) final { val() = visit(expr); }
    void Visit(const NaryExpr& expr) final { val() = visit(expr); }
    void Visit(const Read& expr) final { val() = visit(expr); }
    void Visit(const PushBack& expr) final { val() = visit(expr); }
    void Visit(const Call& expr) final { val() = visit(expr); }
    void Visit(const LoopNode& expr) final { val() = visit(expr); }

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
            if (ctx().in_sym_tbl.find(tmp) == ctx().in_sym_tbl.end()) {
                auto expr = ctx().in_sym_tbl.at(tmp);
                assign(tmp, expr);
            }
            auto value = visit(symbol);
            ctx().sym_val_map[tmp] = value;
        }

        val() = ctx().sym_val_map.at(tmp);
    }

    virtual void assign(Sym sym, Expr expr)
    {
        eval(expr);
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
