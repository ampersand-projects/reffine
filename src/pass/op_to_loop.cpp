#include "reffine/pass/op_to_loop.h"
#include "reffine/pass/reffinepass.h"
#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

shared_ptr<Op> OpToLoop::reffine(Op& op)
{
    // Only support single iterator for now
    ASSERT(op.iters.size() == 1)

    auto ispace = Reffine::Build(op);

    // Map iter to idx
    auto iter = _sym(op.iters[0]->name, op.iters[0]);
    auto idx = _sym(iter->name + "_idx", _idx_t);
    this->map_sym(op.iters[0], iter);
    this->map_sym(idx, idx);
    this->assign(iter, eval(ispace.idx_to_iter(idx)));

    // Bounds and increment
    auto lb = make_shared<LoopMeta>(LoopMetaOp::LB, idx, eval(ispace.iter_to_idx(ispace.lower_bound)));
    auto ub = make_shared<LoopMeta>(LoopMetaOp::UB, idx, eval(ispace.iter_to_idx(ispace.upper_bound)));
    auto incr = make_shared<LoopMeta>(LoopMetaOp::INCR, idx, eval(ispace.idx_incr(idx)));

    vector<Expr> new_outputs;
    for (auto& output : op.outputs) {
        new_outputs.push_back(eval(output));
    }
    
    return _op(
        vector<Sym>{idx},
        _and(_and(lb, ub), incr),
        new_outputs
    );
}

Expr OpToLoop::visit(Op& op)
{
    return this->reffine(op);
}

Expr OpToLoop::visit(Reduce& red)
{
    auto op = this->reffine(red.op);
    return _red(*op, red.init, red.acc);
}

shared_ptr<Func> OpToLoop::Build(shared_ptr<Func> func)
{
    auto new_func = _func(func->name, nullptr, vector<Sym>{});

    OpToLoopCtx ctx(func, new_func);
    OpToLoop optoloop(ctx);
    func->Accept(optoloop);

    return new_func;
}
