#ifndef INCLUDE_REFFINE_PASS_SCALARPASS_H_
#define INCLUDE_REFFINE_PASS_SCALARPASS_H_

#include "reffine/ir/loop.h"
#include "reffine/pass/irclone.h"

namespace reffine {

using ScalarPassCtx = IRCloneCtx;

class LoadStoreExpand : public IRClone {
public:
    explicit LoadStoreExpand(ScalarPassCtx& ctx) : IRClone(ctx) {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

private:
    Expr visit(Load&) final;
    Expr visit(Store&) final;
};

class NewGetElimination : public IRClone {
public:
    NewGetElimination(ScalarPassCtx& ctx) : IRClone(ctx) {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

private:
    Expr visit(Select&) final;
    Expr visit(New&) final;
    Expr visit(Get&) final;
    void visit(Func&) final;

    map<Expr, vector<Expr>> _new_get_map;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_SCALARPASS_H_
