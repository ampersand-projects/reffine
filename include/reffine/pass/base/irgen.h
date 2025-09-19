#ifndef INCLUDE_REFFINE_PASS_BASE_IRGEN_H_
#define INCLUDE_REFFINE_PASS_BASE_IRGEN_H_

#include <map>
#include <memory>
#include <tuple>
#include <utility>

#include "reffine/pass/base/irpass.h"

using namespace std;

namespace reffine {

template <typename CtxTy, typename ValTy>
class IRGenBase : public IRPassBase<CtxTy, ValTy> {
public:
    IRGenBase(unique_ptr<CtxTy> ctx) : IRPassBase<CtxTy, ValTy>(std::move(ctx))
    {
    }

    ValTy eval(Expr expr)
    {
        ValTy new_val;

        swap(new_val, val());
        expr->Accept(*this);
        swap(val(), new_val);

        return new_val;
    }

protected:
    virtual ValTy visit(Sym) { throw runtime_error("Sym visit not supported"); }
    virtual ValTy visit(Select&)
    {
        throw runtime_error("Select visit not supported");
    }
    virtual ValTy visit(IfElse&)
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
    virtual ValTy visit(Stmts&)
    {
        throw runtime_error("Stmts visit not supported");
    }
    virtual ValTy visit(Func&)
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
    virtual ValTy visit(Store&)
    {
        throw runtime_error("Store visit not supported");
    }
    virtual ValTy visit(AtomicOp&)
    {
        throw runtime_error("AtomicOp visit not supported");
    }
    virtual ValTy visit(StructGEP&)
    {
        throw runtime_error("StructGEP visit not supported");
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
    virtual ValTy visit(FetchDataPtr&)
    {
        throw runtime_error("FetchDataPtr visit not supported");
    }
    virtual ValTy visit(NoOp&)
    {
        throw runtime_error("NoOp visit not supported");
    }

    void Visit(Select& expr) final { val() = visit(expr); }
    void Visit(IfElse& stmt) final { val() = visit(stmt); }
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
    void Visit(Stmts& stmt) final { val() = visit(stmt); }
    void Visit(Alloc& expr) final { val() = visit(expr); }
    void Visit(Load& expr) final { val() = visit(expr); }
    void Visit(Store& expr) final { val() = visit(expr); }
    void Visit(AtomicOp& stmt) final { val() = visit(stmt); }
    void Visit(StructGEP& expr) final { val() = visit(expr); }
    void Visit(ThreadIdx& expr) final { val() = visit(expr); }
    void Visit(BlockIdx& expr) final { val() = visit(expr); }
    void Visit(BlockDim& expr) final { val() = visit(expr); }
    void Visit(GridDim& expr) final { val() = visit(expr); }
    void Visit(Loop& expr) final { val() = visit(expr); }
    void Visit(FetchDataPtr& expr) final { val() = visit(expr); }
    void Visit(NoOp& stmt) final { val() = visit(stmt); }
    void Visit(Func& stmt) final { val() = visit(stmt); }
    void Visit(SymNode& symbol) override
    {
        auto sym = this->tmp_sym(symbol);

        if (this->ctx().out_sym_tbl.find(sym) ==
            this->ctx().out_sym_tbl.end()) {
            this->assign(sym, visit(sym));
        }

        val() = this->ctx().out_sym_tbl.at(sym);
    }

protected:
    ValTy& val() { return _val; }

private:
    ValTy _val;
};

template <typename ValTy>
using ValGenCtx = IRPassBaseCtx<ValTy>;

template <typename ValTy>
class ValGen : public IRGenBase<ValGenCtx<ValTy>, ValTy> {
public:
    ValGen(unique_ptr<ValGenCtx<ValTy>> ctx)
        : IRGenBase<ValGenCtx<ValTy>, ValTy>(std::move(ctx))
    {
    }

protected:
    void Visit(SymNode& symbol) final
    {
        auto sym = this->tmp_sym(symbol);
        this->val() = this->visit(sym);
    }
};

class IRGenCtx : public IRPassBaseCtx<Expr> {
public:
    IRGenCtx(const Func& in_func, shared_ptr<Func> out_func)
        : IRPassBaseCtx<Expr>(in_func.tbl, out_func->tbl), out_func(out_func)
    {
    }

    map<Sym, Sym> sym_sym_map;  // mapping from old sym to new sym
    shared_ptr<Func> out_func;
};

class IRGen : public IRGenBase<IRGenCtx, Expr> {
public:
    IRGen(unique_ptr<IRGenCtx> ctx) : IRGenBase<IRGenCtx, Expr>(std::move(ctx))
    {
    }

protected:
    void Visit(SymNode& symbol) final
    {
        auto old_sym = this->tmp_sym(symbol);

        if (this->ctx().sym_sym_map.find(old_sym) ==
            this->ctx().sym_sym_map.end()) {
            auto new_val = eval(this->ctx().in_sym_tbl.at(old_sym));
            auto new_sym = static_pointer_cast<SymNode>(visit(old_sym));
            this->assign(new_sym, new_val);
            this->map_sym(old_sym, new_sym);
        }

        val() = this->ctx().sym_sym_map.at(old_sym);
    }

    virtual void map_sym(Sym old_sym, Sym new_sym)
    {
        this->ctx().sym_sym_map[old_sym] = new_sym;
        this->ctx().sym_sym_map[new_sym] = new_sym;
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_BASE_IRGEN_H_
