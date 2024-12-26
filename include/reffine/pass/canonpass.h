#ifndef INCLUDE_REFFINE_PASS_CANONPASS_H_
#define INCLUDE_REFFINE_PASS_CANONPASS_H_

#include "reffine/ir/loop.h"
#include "reffine/pass/base/irpass.h"

namespace reffine {

class CanonPass : public IRPass {
public:
    explicit CanonPass(IRPassCtx& ctx) : IRPass(ctx) {}

    static void Build(shared_ptr<Func>);

protected:
    void Visit(Loop&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_CANONPASS_H_
