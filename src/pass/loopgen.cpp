#include "reffine/pass/loopgen.h"

#include "reffine/builder/reffiner.h"
#include "reffine/pass/reffinepass.h"
#include "reffine/pass/z3solver.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

Expr LoopGen::visit(Element& elem)
{
    auto vec = eval(elem.vec);
    auto iter = eval(elem.iters[0]);

    auto idx_expr = _call("vector_locate", _idx_t, vector<Expr>{vec, iter});
    auto idx = _sym("elem_idx", idx_expr);
    this->assign(idx, idx_expr);

    vector<Expr> vals;
    for (size_t i = 0; i < vec->type.dtypes.size(); i++) {
        auto data_ptr = _fetch(vec, idx, i);
        auto data = _load(data_ptr);
        vals.push_back(data);
    }

    return _new(vals);
}

OpToLoop LoopGen::op_to_loop(Op& op)
{
    OpToLoop otl;

    // Only support single indexed operations
    ASSERT(op.iters.size() == 1);

    auto ispace = Reffine::Build(op);

    // Loop index allocation
    auto loop_idx_addr_expr = _alloc(_idx_t);
    otl.loop_idx_addr = _sym("loop_idx_addr", loop_idx_addr_expr);
    this->assign(otl.loop_idx_addr, loop_idx_addr_expr);
    this->map_sym(otl.loop_idx_addr, otl.loop_idx_addr);

    // Loop idx symbol
    auto load_loop_idx_expr = _load(otl.loop_idx_addr);
    auto loop_idx = _sym("loop_idx", load_loop_idx_expr);
    this->assign(loop_idx, load_loop_idx_expr);
    this->map_sym(loop_idx, loop_idx);

    // Map op idx to loop idx
    auto op_iter = _sym(op.iters[0]->name, op.iters[0]);
    this->map_sym(op.iters[0], op_iter);
    auto loop_idx_to_op_idx_expr = eval(ispace.idx_to_iter(loop_idx));
    this->assign(op_iter, loop_idx_to_op_idx_expr);

    // Loop init statement
    auto lb_expr = eval(ispace.iter_to_idx(ispace.lower_bound));
    otl.init = _store(otl.loop_idx_addr, lb_expr);

    // Loop exit condition
    auto ub_expr = eval(ispace.iter_to_idx(ispace.upper_bound));
    otl.exit_cond = _gt(loop_idx, ub_expr);

    // Loop index increment expression
    auto incr_expr = eval(ispace.idx_incr(loop_idx));
    otl.incr = _store(otl.loop_idx_addr, incr_expr);

    // Loop body condition
    otl.body_cond = nullptr;

    return otl;
}

Expr LoopGen::visit(Reduce& red)
{
    auto otl = op_to_loop(red.op);

    // State allocation and initialization
    auto state_addr_expr = _alloc(red.type);
    auto state_addr = _sym("state_addr", state_addr_expr);
    this->assign(state_addr, state_addr_expr);
    auto load_state_expr = _load(state_addr);

    // Loop body expression
    vector<Expr> op_outputs;
    for (auto output : red.op.outputs) { op_outputs.push_back(eval(output)); }
    auto val = _new(op_outputs);

    // Loop definition
    auto red_loop = _loop(load_state_expr);
    red_loop->init = _stmts(vector<Stmt>{
        otl.init,
        _store(state_addr, red.init()),
    });
    red_loop->incr = otl.incr;
    red_loop->exit_cond = otl.exit_cond;
    red_loop->body_cond = otl.body_cond;
    red_loop->body = _store(state_addr, red.acc(load_state_expr, val));
    red_loop->post = nullptr;

    return red_loop;
}

shared_ptr<Func> LoopGen::Build(shared_ptr<Func> op_func)
{
    auto loop_func = _func(op_func->name, nullptr, vector<Sym>{});

    LoopGenCtx ctx(op_func, loop_func);
    LoopGen loopgen(ctx);
    op_func->Accept(loopgen);

    return loop_func;
}
