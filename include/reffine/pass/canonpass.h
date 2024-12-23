#ifndef INCLUDE_REFFINE_PASS_CANONPASS_H_
#define INCLUDE_REFFINE_PASS_CANONPASS_H_

#include "reffine/ir/loop.h"
#include "reffine/pass/base/irpass.h"

namespace reffine {

class CanonPass : public IRPass<IRPassCtx> {
public:
    explicit CanonPass(shared_ptr<Func> func) : IRPass(IRPassCtx(func->tbl)) {}

    static void Build(shared_ptr<Func>);

protected:
    void Visit(Loop&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_CANONPASS_H_
