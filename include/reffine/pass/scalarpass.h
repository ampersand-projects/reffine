#ifndef INCLUDE_REFFINE_PASS_SCALARPASS_H_
#define INCLUDE_REFFINE_PASS_SCALARPASS_H_

#include "reffine/ir/loop.h"
#include "reffine/pass/irclone.h"

namespace reffine {

using LoadStoreExpandCtx = IRCloneCtx;

class LoadStoreExpand : public IRClone {
public:
    explicit LoadStoreExpand(LoadStoreExpandCtx& ctx) : IRClone(ctx) {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

protected:
    Expr visit(Load&) final;
    Expr visit(Store&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_SCALARPASS_H_
