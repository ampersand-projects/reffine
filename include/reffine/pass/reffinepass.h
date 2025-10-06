#ifndef INCLUDE_REFFINE_PASS_REFFINEPASS_H_
#define INCLUDE_REFFINE_PASS_REFFINEPASS_H_

#include "reffine/iter/iter_space.h"
#include "reffine/pass/base/irgen.h"
#include "reffine/pass/irclone.h"

namespace reffine {

using ReffineCtx = ValGenCtx<ISpace>;

class Reffine : public ValGen<ISpace> {
public:
    Reffine(unique_ptr<ReffineCtx> ctx, Sym iter)
        : ValGen<ISpace>(std::move(ctx)), _iter(iter)
    {
    }

    static ISpace Build(Sym, Expr, const SymTable&);

private:
    ISpace visit(NaryExpr&) final;
    ISpace visit(Sym) final;
    ISpace visit(Element&) final;
    ISpace visit(NotNull&) final;

    ISpace extract_bound(Sym, NaryExpr&);

    Sym iter() { return _iter; }

    Sym _iter;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_REFFINEPASS_H_
