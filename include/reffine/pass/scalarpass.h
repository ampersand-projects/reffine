#ifndef INCLUDE_REFFINE_PASS_SCALARPASS_H_
#define INCLUDE_REFFINE_PASS_SCALARPASS_H_

#include "reffine/ir/loop.h"
#include "reffine/pass/irclone.h"

namespace reffine {

using ScalarPassCtx = IRCloneCtx;

class ScalarPass : public IRClone {
public:
    explicit ScalarPass(ScalarPassCtx& ctx) : IRClone(ctx) {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

protected:
    Expr visit(Get&) final;
};


class LoadStoreExpand : public IRClone {
public:
    explicit LoadStoreExpand(ScalarPassCtx& ctx) : IRClone(ctx) {}

private:
    Expr visit(Load&) final;
    Expr visit(Store&) final;
};

using NewGetEliminationCtx = ValGenCtx<Expr>;

class NewGetElimination : public ValGen<Expr> {
public:
    NewGetElimination(NewGetEliminationCtx ctx) : ValGen<Expr>(ctx) {}

private:
    Expr visit(Sym);
    Expr visit(New&);
    Expr visit(Get&);

    size_t col;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_SCALARPASS_H_
