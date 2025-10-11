#ifndef INCLUDE_REFFINE_PASS_REFFINEPASS_H_
#define INCLUDE_REFFINE_PASS_REFFINEPASS_H_

#include <set>

#include "reffine/iter/iter_space.h"
#include "reffine/pass/base/irgen.h"
#include "reffine/pass/irclone.h"
#include "reffine/pass/loopgen.h"

namespace reffine {

using ReffineCtx = ValGenCtx<ISpace>;

class Reffine : public ValGen<ISpace> {
public:
    Reffine(unique_ptr<ReffineCtx> ctx) : ValGen<ISpace>(std::move(ctx)) {}

private:
    ISpace visit(NaryExpr&) final;
    ISpace visit(Sym) final;
    ISpace visit(Const&) final;
    ISpace visit(In&) final;
    ISpace visit(Op&) final;

    ISpace extract_bound(NaryExpr&);

    Sym& iter() { return this->_iter; }
    set<Sym>& vars() { return this->_vars; }

    Sym _iter;
    set<Sym> _vars;

    friend class LoopGen;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_REFFINEPASS_H_
