#ifndef INCLUDE_REFFINE_PASS_OP_TO_LOOP_H_
#define INCLUDE_REFFINE_PASS_OP_TO_LOOP_H_

#include "reffine/pass/irclone.h"

namespace reffine {

class OpToLoopCtx : public IRCloneCtx {
public:
    OpToLoopCtx(shared_ptr<Func> old_func, shared_ptr<Func> new_func)
        : IRCloneCtx(old_func, new_func)
    {
    }
};

class OpToLoop : public IRClone {
public:
    explicit OpToLoop(OpToLoopCtx& ctx) : IRClone(ctx) {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

private:
    shared_ptr<Op> reffine(Op&);
    Expr visit(Op&) final;
    Expr visit(Reduce&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_OP_TO_LOOP_H_
