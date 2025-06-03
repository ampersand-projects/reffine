#include "reffine/pass/reffinepass.h"

#include "reffine/builder/reffiner.h"
#include "reffine/pass/z3solver.h"

using namespace reffine;
using namespace reffine::reffiner;

static Expr get_lower_bound(Sym iter, Expr pred)
{
    auto p = _sym(iter->name + "_p", iter);
    auto forall = _forall(iter, _and(_implies(_gte(iter, p), pred),
                                     _implies(_lt(iter, p), _not(pred))));

    Z3Solver solver;
    if (solver.check(forall) == z3::sat) {
        auto p_val = solver.get(p).as_int64();
        return _const(iter->type, p_val);
    }

    return nullptr;
}

static Expr get_upper_bound(Sym iter, Expr pred)
{
    auto p = _sym(iter->name + "_p", iter);
    auto forall = _forall(iter, _and(_implies(_lte(iter, p), pred),
                                     _implies(_gt(iter, p), _not(pred))));

    Z3Solver solver;
    if (solver.check(forall) == z3::sat) {
        auto p_val = solver.get(p).as_int64();
        return _const(iter->type, p_val);
    }

    return nullptr;
}

ISpace Reffine::visit(NaryExpr& e)
{
    switch (e.op) {
        case MathOp::AND:
            return eval(e.arg(0)) & eval(e.arg(1));
        case MathOp::OR:
            return eval(e.arg(0)) | eval(e.arg(1));
        case MathOp::LT:
        case MathOp::LTE:
        case MathOp::GT:
        case MathOp::GTE: {
            auto iter = op().iters[0];
            auto pred = this->tmp_expr(e);

            if (auto lb = get_lower_bound(iter, pred)) {
                return make_shared<LBoundSpace>(eval(iter), lb);
            } else if (auto ub = get_upper_bound(iter, pred)) {
                return make_shared<UBoundSpace>(eval(iter), ub);
            } else {
                throw runtime_error("Unabled to identify the bounds");
            }
        }
        default:
            throw runtime_error("Operator not supported by Reffine");
    }
}

ISpace Reffine::visit(Sym sym)
{
    if (sym == op().iters[0]) {
        // return universal space for operator iterator
        return make_shared<IterSpace>(sym->type);
    } else if (sym->type.is_vector()) {
        return make_shared<VecSpace>(sym);
    } else {
        throw runtime_error("Unrecognized symbol in reffine pass");
    }
}

ISpace Reffine::visit(Element& elem)
{
    ASSERT(elem.iters.size() == 1);
    ASSERT(elem.iters[0] == op().iters[0]);

    // Assuming Element is only visited through NotNull
    // Therefore, always returning vector space
    return eval(elem.vec);
}

ISpace Reffine::visit(NotNull& not_null)
{
    // Assuming NotNull always traslates to vector space
    return eval(not_null.elem);
}

ISpace Reffine::Build(Op& op)
{
    // Only support single iterator for now
    ASSERT(op.iters.size() == 1);

    ReffineCtx ctx;
    Reffine rpass(ctx, op);

    return rpass.eval(op.pred);
}
