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
    virtual void visit(const IfElse&) = 0;
    virtual ValTy visit(const Exists&) = 0;
    virtual ValTy visit(const Const&) = 0;
    virtual ValTy visit(const Cast&) = 0;
    virtual ValTy visit(const NaryExpr&) = 0;
    virtual ValTy visit(const Read&) = 0;
    virtual ValTy visit(const Write&) = 0;
    virtual ValTy visit(const Call&) = 0;
    virtual void visit(const Stmts&) = 0;
    virtual void visit(const Func&) = 0;
    virtual ValTy visit(const Alloc&) = 0;
    virtual ValTy visit(const Load&) = 0;
    virtual void visit(const Store&) = 0;
    virtual ValTy visit(const Loop&) = 0;
    virtual ValTy visit(const IsValid&) = 0;
    virtual ValTy visit(const SetValid&) = 0;
    virtual ValTy visit(const FetchDataPtr&) = 0;
    virtual void visit(const NoOp&) = 0;

    void Visit(const Select& expr) final { val() = visit(expr); }
    void Visit(const IfElse& stmt) final { visit(stmt); val() = nullptr; }
    void Visit(const Exists& expr) final { val() = visit(expr); }
    void Visit(const Const& expr) final { val() = visit(expr); }
    void Visit(const Cast& expr) final { val() = visit(expr); }
    void Visit(const NaryExpr& expr) final { val() = visit(expr); }
    void Visit(const Read& expr) final { val() = visit(expr); }
    void Visit(const Write& expr) final { val() = visit(expr); }
    void Visit(const Call& expr) final { val() = visit(expr); }
    void Visit(const Stmts& stmt) final { visit(stmt); val() = nullptr; }
    void Visit(const Func& stmt) final { visit(stmt); val() = nullptr; }
    void Visit(const Alloc& expr) final { val() = visit(expr); }
    void Visit(const Load& expr) final { val() = visit(expr); }
    void Visit(const Store& expr) final { visit(expr); val() = nullptr; }
    void Visit(const Loop& expr) final { val() = visit(expr); }
    void Visit(const IsValid& expr) final { val() = visit(expr); }
    void Visit(const SetValid& expr) final { val() = visit(expr); }
    void Visit(const FetchDataPtr& expr) final { val() = visit(expr); }
    void Visit(const NoOp& stmt) final { visit(stmt); val() = nullptr; }

    CtxTy& switch_ctx(CtxTy& new_ctx) { swap(new_ctx, ctx()); return new_ctx; }

    ValTy& val() { return ctx().val; }

    ValTy eval(const Stmt stmt)
    {
        ValTy new_val = nullptr;

        swap(new_val, val());
        stmt->Accept(*this);
        swap(val(), new_val);

        return new_val;
    }

    void Visit(const SymNode& symbol) final
    {
        auto tmp = tmp_sym(symbol);

        if (ctx().sym_val_map.find(tmp) == ctx().sym_val_map.end()) {
            auto expr = ctx().in_sym_tbl.at(tmp);
            assign(tmp, eval(expr));
        }

        val() = ctx().sym_val_map.at(tmp);
    }

    virtual void assign(Sym sym, ValTy val)
    {
        ctx().sym_val_map[sym] = val;
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
