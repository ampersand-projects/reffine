#ifndef INCLUDE_REFFINE_PASS_REFFINEPASS_H_
#define INCLUDE_REFFINE_PASS_REFFINEPASS_H_

#include "reffine/pass/base/irgen.h"
#include "reffine/pass/irclone.h"
#include "reffine/iter/iter_space.h"

namespace reffine {

using ReffineCtx = ValGenCtx<ISpace>;

class Reffine : public ValGen<ISpace> {
public:
    Reffine(ReffineCtx& ctx, Op& op) : ValGen<ISpace>(ctx), _op(op) {}

    static ISpace Build(Op&);

private:
    ISpace visit(NaryExpr&) final;
    ISpace visit(Sym) final;
    ISpace visit(Element&) final;
    ISpace visit(NotNull&) final;

    Op& op() { return _op; }

    Op& _op;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_REFFINEPASS_H_
