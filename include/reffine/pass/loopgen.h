#ifndef INCLUDE_REFFINE_PASS_LOOPGEN_H_
#define INCLUDE_REFFINE_PASS_LOOPGEN_H_

#include "reffine/pass/irclone.h"

namespace reffine {

class LoopGen : public IRClone {
public:
    LoopGen(unique_ptr<IRGenCtx> ctx = nullptr, bool vectorize = true)
        : IRClone(std::move(ctx)), _vectorize(vectorize)
    {
    }

private:
    shared_ptr<Loop> build_loop(Op&, shared_ptr<Loop>);
    Expr visit(Op&) final;
    Expr visit(Reduce&) final;
    Expr visit(Element&) final;
    Expr visit(Lookup&) final;

    map<Expr, map<Expr, Expr>> _vec_iter_idx_map;  // vec -> iter -> idx
    bool _vectorize;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_LOOPGEN_H_
