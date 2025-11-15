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

    IRPassBaseCtx(const SymTable& in_sym_tbl,
                  unique_ptr<map<Sym, ValTy>> out_sym_tbl_ptr =
                      make_unique<map<Sym, ValTy>>())
        : in_sym_tbl(in_sym_tbl),
          out_sym_tbl(*out_sym_tbl_ptr),
          out_sym_tbl_ptr(std::move(out_sym_tbl_ptr))
    {
    }

    IRPassBaseCtx(unique_ptr<SymTable> in_sym_tbl_ptr = make_unique<SymTable>(),
                  unique_ptr<map<Sym, ValTy>> out_sym_tbl_ptr =
                      make_unique<map<Sym, ValTy>>())
        : in_sym_tbl(*in_sym_tbl_ptr),
          out_sym_tbl(*out_sym_tbl_ptr),
          in_sym_tbl_ptr(std::move(in_sym_tbl_ptr)),
          out_sym_tbl_ptr(std::move(out_sym_tbl_ptr))
    {
    }

    const SymTable& in_sym_tbl;
    map<Sym, ValTy>& out_sym_tbl;

private:
    unique_ptr<SymTable> in_sym_tbl_ptr;
    unique_ptr<map<Sym, ValTy>> out_sym_tbl_ptr;
};

template <typename CtxTy, typename ValTy>
class IRPassBase : public Visitor {
public:
    explicit IRPassBase(unique_ptr<CtxTy> ctx) : _ctx(std::move(ctx)) {}

    CtxTy& ctx() { return *this->_ctx; }

protected:
    virtual void assign(Sym sym, ValTy val)
    {
        this->ctx().out_sym_tbl[sym] = val;
    }

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

    void switch_ctx(unique_ptr<CtxTy>& octx) { std::swap(octx, this->_ctx); }

    unique_ptr<CtxTy> _ctx;
};

using IRPassCtx = IRPassBaseCtx<Sym>;

class IRPass : public IRPassBase<IRPassCtx, Sym> {
public:
    explicit IRPass(unique_ptr<IRPassCtx> ctx)
        : IRPassBase<IRPassCtx, Sym>(std::move(ctx))
    {
    }

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
        for (auto& iter : expr.iters) { this->assign(iter, iter); }
        expr.pred->Accept(*this);
        for (auto& output : expr.outputs) { output->Accept(*this); }
    }

    void Visit(Element& expr) override
    {
        expr.vec->Accept(*this);
        expr.iter->Accept(*this);
    }

    void Visit(Lookup& expr) override
    {
        expr.vec->Accept(*this);
        expr.idx->Accept(*this);
    }

    void Visit(In& expr) override
    {
        expr.iter->Accept(*this);
        expr.vec->Accept(*this);
    }

    void Visit(Reduce& expr) override { expr.vec->Accept(*this); }

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
        for (auto& input : stmt.inputs) { this->assign(input, input); }

        stmt.output->Accept(*this);
    }

    void Visit(Alloc& expr) override { expr.size->Accept(*this); }

    void Visit(Load& expr) override
    {
        expr.addr->Accept(*this);
        expr.offset->Accept(*this);
    }

    void Visit(Store& expr) override
    {
        expr.addr->Accept(*this);
        expr.val->Accept(*this);
        expr.offset->Accept(*this);
    }

    void Visit(AtomicOp& stmt) override
    {
        stmt.addr->Accept(*this);
        stmt.val->Accept(*this);
    }

    void Visit(StructGEP& expr) override { expr.addr->Accept(*this); }

    void Visit(ThreadIdx& expr) override {}
    void Visit(BlockDim& expr) override {}
    void Visit(BlockIdx& expr) override {}
    void Visit(GridDim& expr) override {}

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

    void Visit(FetchDataPtr& expr) override { expr.vec->Accept(*this); }

    void Visit(NoOp& stmt) override {}

    void Visit(Define& define) override
    {
        define.sym->Accept(*this);
        define.val->Accept(*this);
    }

    void Visit(InitVal& initval) override
    {
        initval.val->Accept(*this);
        for (auto init : initval.inits) { init->Accept(*this); }
    }

    void Visit(ReadData& expr) override
    {
        expr.vec->Accept(*this);
        expr.idx->Accept(*this);
    }

    void Visit(WriteData& expr) override
    {
        expr.vec->Accept(*this);
        expr.idx->Accept(*this);
        expr.val->Accept(*this);
    }

    void Visit(ReadBit& expr) override
    {
        expr.vec->Accept(*this);
        expr.idx->Accept(*this);
    }

    void Visit(WriteBit& expr) override
    {
        expr.vec->Accept(*this);
        expr.idx->Accept(*this);
        expr.val->Accept(*this);
    }

    void Visit(Length& expr) override { expr.vec->Accept(*this); }

    void Visit(SubVector& expr) override
    {
        expr.vec->Accept(*this);
        expr.start->Accept(*this);
        expr.end->Accept(*this);
    }

protected:
    void Visit(SymNode& symbol) override
    {
        auto tmp = tmp_sym(symbol);

        if (ctx().out_sym_tbl.find(tmp) == ctx().out_sym_tbl.end()) {
            ctx().in_sym_tbl.at(tmp)->Accept(*this);
            this->assign(tmp, tmp);
        }
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_BASE_IRPASS_H_
