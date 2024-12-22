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
    Expr visit(Reduce&) final;
};


class OpToLoop : public IRGen<Sym, Expr> {
public:
    explicit OpToLoop(IRGenCtx<Sym, Expr>& ctx) : IRGen(ctx) {}

private:
    Expr visit(Op&) override;

    Sym idx;
    Expr lower_bound;
    Expr upper_bound;
    Expr next_idx;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_LOOPGEN_H_
