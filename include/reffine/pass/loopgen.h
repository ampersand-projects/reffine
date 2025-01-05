#ifndef INCLUDE_REFFINE_PASS_LOOPGEN_H_
#define INCLUDE_REFFINE_PASS_LOOPGEN_H_

#include "reffine/pass/irclone.h"

namespace reffine {

using LoopGenCtx = IRCloneCtx;

class LoopGen : public IRClone {
public:
    explicit LoopGen(LoopGenCtx& ctx) : IRClone(ctx) {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

private:
    shared_ptr<Loop> build_loop(Op&);
    Expr visit(Reduce&) final;
    Expr visit(Element&) final;
    Expr visit(Lookup&) final;
    Expr visit(NotNull&) final
    {
        throw runtime_error("NotNull visit not supported");
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_LOOPGEN_H_
