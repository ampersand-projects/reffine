#include "reffine/pass/loopgen.h"

#include "reffine/pass/reffinepass.h"
#include "reffine/pass/z3solver.h"

using namespace std;
using namespace reffine;

OpToLoop LoopGen::op_to_loop(Op& op)
{
    OpToLoop otl;

    // Only support single indexed operations
    ASSERT(op.iters.size() == 1);

    auto ispace = ReffinePass::Build(op);

    // Loop index allocation
    auto loop_idx_addr_expr = make_shared<Alloc>(types::IDX);
    otl.loop_idx_addr =
        make_shared<SymNode>("loop_idx_addr", loop_idx_addr_expr);
    this->assign(otl.loop_idx_addr, loop_idx_addr_expr);
    auto load_loop_idx_expr = make_shared<Load>(otl.loop_idx_addr);

    // Map op idx to loop idx
    auto op_iter = make_shared<SymNode>(op.iters[0]->name, op.iters[0]);
    this->map_sym(op.iters[0], op_iter);
    auto loop_idx_to_op_idx_expr = ispace.idx_to_iter(load_loop_idx_expr);
    this->assign(op_iter, loop_idx_to_op_idx_expr);

    // Loop init statement
    otl.init = make_shared<Store>(otl.loop_idx_addr,
                                  ispace.iter_to_idx(ispace.lower_bound));

    // Loop exit condition
    otl.exit_cond = make_shared<GreaterThan>(
        load_loop_idx_expr, ispace.iter_to_idx(ispace.upper_bound));

    // Loop index increment expression
    otl.incr = make_shared<Store>(otl.loop_idx_addr,
                                  ispace.idx_incr(load_loop_idx_expr));

    // Loop body condition
    otl.body_cond = ispace.body_cond;

    return otl;
}

Expr LoopGen::visit(Reduce& red)
{
    auto otl = op_to_loop(red.op);

    // State allocation and initialization
    auto state_addr_expr = make_shared<Alloc>(red.type);
    auto state_addr = make_shared<SymNode>("state_addr", state_addr_expr);
    this->assign(state_addr, state_addr_expr);
    auto load_state_expr = make_shared<Load>(state_addr);

    // Loop body expression
    vector<Expr> op_outputs;
    for (auto output : red.op.outputs) { op_outputs.push_back(eval(output)); }
    auto val = make_shared<New>(op_outputs);

    // Loop definition
    auto red_loop = make_shared<Loop>(load_state_expr);
    red_loop->init = make_shared<Stmts>(vector<Stmt>{
        otl.init,
        make_shared<Store>(state_addr, red.init()),
    });
    red_loop->incr = otl.incr;
    red_loop->exit_cond = otl.exit_cond;
    red_loop->body_cond = otl.body_cond;
    red_loop->body =
        make_shared<Store>(state_addr, red.acc(load_state_expr, val));
    red_loop->post = nullptr;

    return red_loop;
}

shared_ptr<Func> LoopGen::Build(shared_ptr<Func> op_func)
{
    auto loop_func = make_shared<Func>(op_func->name, nullptr, vector<Sym>{});

    LoopGenCtx ctx(op_func, loop_func);
    LoopGen loopgen(ctx);
    op_func->Accept(loopgen);

    return loop_func;
}
