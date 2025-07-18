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

private:
    map<Expr, map<Expr, Expr>> vec_iter_idx_map;  // vec -> iter -> idx

    friend class LoopGen;
};

class LoopGen : public IRClone {
public:
    explicit LoopGen(LoopGenCtx& ctx) : IRClone(ctx), _loopgenctx(ctx) {}

    static shared_ptr<Func> Build(shared_ptr<Func>);

private:
    shared_ptr<Loop> build_loop(Op&);
    Expr visit(Reduce&) final;
    Expr visit(Element&) final;
    Expr visit(NotNull&) final
    {
        throw runtime_error("NotNull visit not supported");
    }

    LoopGenCtx& _loopgenctx;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_LOOPGEN_H_
