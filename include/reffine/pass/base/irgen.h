#ifndef INCLUDE_REFFINE_PASS_BASE_IRGEN_H_
#define INCLUDE_REFFINE_PASS_BASE_IRGEN_H_

#include <map>
#include <memory>
#include <tuple>
#include <utility>

#include "reffine/pass/base/irpass.h"

using namespace std;

namespace reffine {

template <typename ValTy>
class IRGenBase : public IRPassBase<ValTy> {
public:
    IRGenBase(IRPassBaseCtx<ValTy>& ctx) : IRPassBase<ValTy>(ctx) {}

protected:
    virtual ValTy visit(Sym) { throw runtime_error("Sym visit not supported"); }
    virtual ValTy visit(Select&)
    {
        throw runtime_error("Select visit not supported");
    }
    virtual void visit(IfElse&)
    {
        throw runtime_error("IfElse visit not supported");
    }
    virtual ValTy visit(Const&)
    {
        throw runtime_error("Const visit not supported");
    }
    virtual ValTy visit(Cast&)
    {
        throw runtime_error("Cast visit not supported");
    }
    virtual ValTy visit(Get&)
    {
        throw runtime_error("Get visit not supported");
    }
    virtual ValTy visit(New&)
    {
        throw runtime_error("New visit not supported");
    }
    virtual ValTy visit(NaryExpr&)
    {
        throw runtime_error("NaryExpr visit not supported");
    }
    virtual ValTy visit(Op&) { throw runtime_error("Op visit not supported"); }
    virtual ValTy visit(Element&)
    {
        throw runtime_error("Element visit not supported");
    }
    virtual ValTy visit(NotNull&)
    {
        throw runtime_error("NotNull visit not supported");
    }
    virtual ValTy visit(Reduce&)
    {
        throw runtime_error("Reduce visit not supported");
    }
    virtual ValTy visit(Call&)
    {
        throw runtime_error("Call visit not supported");
    }
    virtual void visit(Stmts&)
    {
        throw runtime_error("Stmts visit not supported");
    }
    virtual void visit(Func&)
    {
        throw runtime_error("Func visit not supported");
    }
    virtual ValTy visit(Alloc&)
    {
        throw runtime_error("Alloc visit not supported");
    }
    virtual ValTy visit(Load&)
    {
        throw runtime_error("Load visit not supported");
    }
    virtual void visit(Store&)
    {
        throw runtime_error("Store visit not supported");
    }
    virtual ValTy visit(ThreadIdx&)
    {
        throw runtime_error("ThreadIdx visit not supported");
    }
    virtual ValTy visit(BlockIdx&)
    {
        throw runtime_error("BlockIdx visit not supported");
    }
    virtual ValTy visit(BlockDim&)
    {
        throw runtime_error("BlockDim visit not supported");
    }
    virtual ValTy visit(GridDim&)
    {
        throw runtime_error("GridDim visit not supported");
    }
    virtual ValTy visit(Loop&)
    {
        throw runtime_error("Loop visit not supported");
    }
    virtual ValTy visit(IsValid&)
    {
        throw runtime_error("IsValid visit not supported");
    }
    virtual ValTy visit(SetValid&)
    {
        throw runtime_error("SetValid visit not supported");
    }
    virtual ValTy visit(FetchDataPtr&)
    {
        throw runtime_error("FetchDataPtr visit not supported");
    }
    virtual void visit(NoOp&)
    {
        throw runtime_error("NoOp visit not supported");
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
    void Visit(NotNull& expr) final { val() = visit(expr); }
    void Visit(Reduce& expr) final { val() = visit(expr); }
    void Visit(Call& expr) final { val() = visit(expr); }
    void Visit(Stmts& stmt) final { visit(stmt); }
    void Visit(Func& stmt) final { visit(stmt); }
    void Visit(Alloc& expr) final { val() = visit(expr); }
    void Visit(Load& expr) final { val() = visit(expr); }
    void Visit(Store& expr) final { visit(expr); }
    void Visit(ThreadIdx& expr) final { val() = visit(expr); }
    void Visit(BlockIdx& expr) final { val() = visit(expr); }
    void Visit(BlockDim& expr) final { val() = visit(expr); }
    void Visit(GridDim& expr) final { val() = visit(expr); }
    void Visit(Loop& expr) final { val() = visit(expr); }
    void Visit(IsValid& expr) final { val() = visit(expr); }
    void Visit(SetValid& expr) final { val() = visit(expr); }
    void Visit(FetchDataPtr& expr) final { val() = visit(expr); }
    void Visit(NoOp& stmt) final { visit(stmt); }
    void Visit(SymNode& symbol) override
    {
        auto sym = this->tmp_sym(symbol);

        if (this->ctx().out_sym_tbl.find(sym) ==
            this->ctx().out_sym_tbl.end()) {
            this->assign(sym, visit(sym));
        }

        val() = this->ctx().out_sym_tbl.at(sym);
    }

    ValTy eval(Stmt stmt)
    {
        ValTy new_val;

        swap(new_val, val());
        stmt->Accept(*this);
        swap(val(), new_val);

        return new_val;
    }

protected:
    ValTy& val() { return _val; }

private:
    ValTy _val;
};

template <typename ValTy>
class ValGenCtx : public IRPassBaseCtx<ValTy> {
public:
    ValGenCtx(SymTable tmp1 = {}, map<Sym, ValTy> tmp2 = {})
        : IRPassBaseCtx<ValTy>(tmp1, tmp2)
    {
    }
};

template <typename ValTy>
class ValGen : public IRGenBase<ValTy> {
public:
    ValGen(ValGenCtx<ValTy>& ctx) : IRGenBase<ValTy>(ctx) {}

protected:
    void Visit(SymNode& symbol) final
    {
        auto sym = this->tmp_sym(symbol);
        this->val() = this->visit(sym);
    }
};

class IRGenCtx : public IRPassBaseCtx<Expr> {
public:
    IRGenCtx(const SymTable& in_sym_tbl, map<Sym, Expr>& out_sym_tbl)
        : IRPassBaseCtx<Expr>(in_sym_tbl, out_sym_tbl)
    {
    }

    map<Sym, Sym> sym_sym_map;  // mapping from old sym to new sym
};

class IRGen : public IRGenBase<Expr> {
public:
    IRGen(IRGenCtx& ctx) : IRGenBase<Expr>(ctx), _irgenctx(ctx) {}

protected:
    void Visit(SymNode& symbol) final
    {
        auto old_sym = this->tmp_sym(symbol);

        if (_irgenctx.sym_sym_map.find(old_sym) ==
            _irgenctx.sym_sym_map.end()) {
            auto new_val = eval(_irgenctx.in_sym_tbl.at(old_sym));
            auto new_sym = static_pointer_cast<SymNode>(visit(old_sym));
            this->assign(new_sym, new_val);
            this->map_sym(old_sym, new_sym);
        }

        val() = _irgenctx.sym_sym_map.at(old_sym);
    }

    virtual void map_sym(Sym old_sym, Sym new_sym)
    {
        _irgenctx.sym_sym_map[old_sym] = new_sym;
    }

private:
    IRGenCtx _irgenctx;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_BASE_IRGEN_H_
