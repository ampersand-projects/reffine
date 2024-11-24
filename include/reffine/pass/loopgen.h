#ifndef INCLUDE_REFFINE_PASS_LOOPGEN_H_
#define INCLUDE_REFFINE_PASS_LOOPGEN_H_

#include "reffine/pass/irclone.h"

namespace reffine {

using LoopGenCtx = IRCloneCtx;

class LoopGen : public IRClone {
public:
    explicit LoopGen(LoopGenCtx& ctx) :
        IRClone(ctx)
    {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

private:
    Expr visit(Reduce&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_LOOPGEN_H_
