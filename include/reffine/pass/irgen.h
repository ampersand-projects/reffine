#ifndef INCLUDE_REFFINE_PASS_IRGEN_H_
#define INCLUDE_REFFINE_PASS_IRGEN_H_

#include <map>
#include <memory>
#include <utility>

#include "reffine/pass/visitor.h"

using namespace std;

namespace reffine {

template<typename ValTy>
class IRGenCtx {
public:
    IRGenCtx(SymTable& in_sym_tbl) :
        in_sym_tbl(in_sym_tbl)
    {}

    SymTable& in_sym_tbl;
    map<Sym, ValTy> sym_val_map;
    ValTy val;
};

template<typename CtxTy, typename ValTy>
class IRGen : public Visitor {
protected:
    virtual CtxTy& ctx() = 0;

    virtual ValTy visit(Select&) = 0;
    virtual void visit(IfElse&) = 0;
    virtual ValTy visit(Exists&) = 0;
    virtual ValTy visit(Const&) = 0;
    virtual ValTy visit(Cast&) = 0;
    virtual ValTy visit(NaryExpr&) = 0;
    virtual ValTy visit(Read&) = 0;
    virtual ValTy visit(Write&) = 0;
    virtual ValTy visit(Call&) = 0;
    virtual void visit(Stmts&) = 0;
    virtual void visit(Func&) = 0;
    virtual ValTy visit(Alloc&) = 0;
    virtual ValTy visit(Load&) = 0;
    virtual void visit(Store&) = 0;
    virtual ValTy visit(Loop&) = 0;
    virtual ValTy visit(IsValid&) = 0;
    virtual ValTy visit(SetValid&) = 0;
    virtual ValTy visit(FetchDataPtr&) = 0;
    virtual void visit(NoOp&) = 0;

    void Visit(Select& expr) final { val() = visit(expr); }
    void Visit(IfElse& stmt) final { visit(stmt); val() = nullptr; }
    void Visit(Exists& expr) final { val() = visit(expr); }
    void Visit(Const& expr) final { val() = visit(expr); }
    void Visit(Cast& expr) final { val() = visit(expr); }
    void Visit(NaryExpr& expr) final { val() = visit(expr); }
    void Visit(Read& expr) final { val() = visit(expr); }
    void Visit(Write& expr) final { val() = visit(expr); }
    void Visit(Call& expr) final { val() = visit(expr); }
    void Visit(Stmts& stmt) final { visit(stmt); val() = nullptr; }
    void Visit(Func& stmt) final { visit(stmt); val() = nullptr; }
    void Visit(Alloc& expr) final { val() = visit(expr); }
    void Visit(Load& expr) final { val() = visit(expr); }
    void Visit(Store& expr) final { visit(expr); val() = nullptr; }
    void Visit(Loop& expr) final { val() = visit(expr); }
    void Visit(IsValid& expr) final { val() = visit(expr); }
    void Visit(SetValid& expr) final { val() = visit(expr); }
    void Visit(FetchDataPtr& expr) final { val() = visit(expr); }
    void Visit(NoOp& stmt) final { visit(stmt); val() = nullptr; }

    CtxTy& switch_ctx(CtxTy& new_ctx) { swap(new_ctx, ctx()); return new_ctx; }

    ValTy& val() { return ctx().val; }

    ValTy eval(Stmt stmt)
    {
        ValTy new_val = nullptr;

        swap(new_val, val());
        stmt->Accept(*this);
        swap(val(), new_val);

        return new_val;
    }

    void Visit(SymNode& symbol) final
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
    Sym tmp_sym(SymNode& symbol)
    {
        shared_ptr<SymNode> tmp_sym(const_cast<SymNode*>(&symbol), [](SymNode*) {});
        return tmp_sym;
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_IRGEN_H_
