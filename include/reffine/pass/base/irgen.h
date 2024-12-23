#ifndef INCLUDE_REFFINE_PASS_BASE_IRGEN_H_
#define INCLUDE_REFFINE_PASS_BASE_IRGEN_H_

#include <map>
#include <memory>
#include <tuple>
#include <utility>

#include "reffine/pass/base/irpass.h"

using namespace std;

namespace reffine {

template <typename SymTy, typename ValTy>
class IRGenCtx : public IRPassCtx {
public:
    IRGenCtx(const SymTable& in_sym_tbl, map<SymTy, ValTy>& out_sym_tbl)
        : IRPassCtx(in_sym_tbl), out_sym_tbl(out_sym_tbl)
    {
    }

    map<Sym, SymTy> sym_sym_map;     // mapping from old sym to new sym
    map<SymTy, ValTy>& out_sym_tbl;  // mapping from new sym to value
};

template <typename SymTy, typename ValTy>
class IRGen : public IRPass<IRGenCtx<SymTy, ValTy>> {
public:
    IRGen(IRGenCtx<SymTy, ValTy>& ctx) : IRPass<IRGenCtx<SymTy, ValTy>>(ctx) {}

protected:
    virtual tuple<SymTy, ValTy> visit(Sym, Expr)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(Select&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void visit(IfElse&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(Const&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(Cast&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(Get&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(New&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(NaryExpr&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(Op&) { throw runtime_error("Operation not supported"); }
    virtual ValTy visit(Element&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(Reduce&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(Call&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void visit(Stmts&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void visit(Func&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(Alloc&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(Load&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void visit(Store&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(Loop&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(IsValid&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(SetValid&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual ValTy visit(FetchDataPtr&)
    {
        throw runtime_error("Operation not supported");
    }
    virtual void visit(NoOp&)
    {
        throw runtime_error("Operation not supported");
    }

    void Visit(Select& expr) final { val() = visit(expr); }
    void Visit(IfElse& stmt) final { visit(stmt); }
    void Visit(Const& expr) final { val() = visit(expr); }
    void Visit(Cast& expr) final { val() = visit(expr); }
    void Visit(Get& expr) final { val() = visit(expr); }
    void Visit(New& expr) final { val() = visit(expr); }
    void Visit(NaryExpr& expr) final { val() = visit(expr); }
    void Visit(Op& expr) final { val() = visit(expr); }
    void Visit(Element& expr) final { val() = visit(expr); }
    void Visit(Reduce& expr) final { val() = visit(expr); }
    void Visit(Call& expr) final { val() = visit(expr); }
    void Visit(Stmts& stmt) final { visit(stmt); }
    void Visit(Func& stmt) final { visit(stmt); }
    void Visit(Alloc& expr) final { val() = visit(expr); }
    void Visit(Load& expr) final { val() = visit(expr); }
    void Visit(Store& expr) final { visit(expr); }
    void Visit(Loop& expr) final { val() = visit(expr); }
    void Visit(IsValid& expr) final { val() = visit(expr); }
    void Visit(SetValid& expr) final { val() = visit(expr); }
    void Visit(FetchDataPtr& expr) final { val() = visit(expr); }
    void Visit(NoOp& stmt) final { visit(stmt); }
    void Visit(SymNode& symbol) final
    {
        auto old_sym = this->tmp_sym(symbol);

        if (this->ctx().sym_sym_map.find(old_sym) ==
            this->ctx().sym_sym_map.end()) {
            auto old_val = this->ctx().in_sym_tbl.at(old_sym);
            auto [new_sym, new_val] = visit(old_sym, old_val);
            map_sym(old_sym, new_sym);
            map_val(new_sym, new_val);
        }

        val() = this->ctx().sym_sym_map.at(old_sym);
    }

    ValTy eval(Stmt stmt)
    {
        ValTy new_val;

        swap(new_val, val());
        stmt->Accept(*this);
        swap(val(), new_val);

        return new_val;
    }

    virtual void map_val(SymTy new_sym, ValTy val)
    {
        this->ctx().out_sym_tbl[new_sym] = val;
    }

    virtual void map_sym(Sym old_sym, SymTy new_sym)
    {
        this->ctx().sym_sym_map[old_sym] = new_sym;
    }

protected:
    ValTy& val() { return _val; }

private:
    ValTy _val;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_BASE_IRGEN_H_
