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
    IRPassCtx(const SymTable& in_sym_tbl) :
        in_sym_tbl(in_sym_tbl)
    {}

    const SymTable& in_sym_tbl;
    set<Sym> sym_set;
};

template<typename CtxTy>
class IRPass : public Visitor {
protected:
    virtual CtxTy& ctx() = 0;

    CtxTy& switch_ctx(CtxTy& new_ctx) { swap(new_ctx, ctx()); return new_ctx; }

    void eval(const Stmt stmt) { stmt->Accept(*this); }

    void Visit(const SymNode& symbol) final
    {
        auto tmp = tmp_sym(symbol);

        if (ctx().sym_set.find(tmp) == ctx().sym_set.end()) {
            eval(ctx().in_sym_tbl.at(tmp));
            assign(tmp);
        }
    }

    virtual void assign(Sym sym)
    {
        ctx().sym_set.insert(tmp);
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
