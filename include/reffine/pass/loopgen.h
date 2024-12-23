#ifndef INCLUDE_REFFINE_PASS_LOOPGEN_H_
#define INCLUDE_REFFINE_PASS_LOOPGEN_H_

#include "reffine/pass/irclone.h"

namespace reffine {

class LoopGenCtx : public IRCloneCtx {
public:
    LoopGenCtx(shared_ptr<Func> old_func, shared_ptr<Func> new_func)
        : IRCloneCtx(old_func, new_func)
    {
    }
};

struct OpToLoop {
    Sym op_idx;
    Sym loop_idx_addr;
    Stmt init;
    Expr exit_cond;
    Stmt incr;
};

class LoopGen : public IRClone {
public:
    explicit LoopGen(LoopGenCtx& ctx) : IRClone(ctx) {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

private:
    OpToLoop op_to_loop(Op&);
    Expr visit(Reduce&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_LOOPGEN_H_
