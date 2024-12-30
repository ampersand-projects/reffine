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

struct OpToLoop2 {
    Sym loop_idx_addr;
    Stmt init;
    Expr exit_cond;
    Expr body_cond;
    Stmt incr;
};

class LoopGen : public IRClone {
public:
    explicit LoopGen(LoopGenCtx& ctx) : IRClone(ctx) {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

private:
    OpToLoop2 op_to_loop(Op&);
    Expr visit(Reduce&) final;
    Expr visit(Element&) final;
    Expr visit(NotNull&) final
    {
        throw runtime_error("NotNull visit not supported");
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_LOOPGEN_H_
