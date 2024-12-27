#include "reffine/pass/loopgen.h"

#include "reffine/pass/z3solver.h"

using namespace std;
using namespace reffine;

Expr get_init_val(Sym idx, Expr pred)
{
    auto p = make_shared<SymNode>(idx->name + "_p", idx);
    auto forall = make_shared<ForAll>(
            idx,
            make_shared<And>(make_shared<Implies>(
                    make_shared<GreaterThanEqual>(idx, p), pred),
                make_shared<Implies>(make_shared<LessThan>(idx, p),
                    make_shared<Not>(pred))));

    Z3Solver solver;
    if (solver.check(forall) == z3::sat) {
        auto p_val = solver.get(p).as_int64();
        return make_shared<Const>(idx->type.btype, p_val);
    }

    return nullptr;
}

Expr get_exit_val(Sym idx, Expr pred)
{
    auto p = make_shared<SymNode>(idx->name + "_p", idx);
    auto forall = make_shared<ForAll>(
            idx,
            make_shared<And>(
                make_shared<Implies>(make_shared<LessThanEqual>(idx, p), pred),
                make_shared<Implies>(make_shared<GreaterThan>(idx, p),
                    make_shared<Not>(pred))));

    Z3Solver solver;
    if (solver.check(forall) == z3::sat) {
        auto p_val = solver.get(p).as_int64();
        return make_shared<Const>(idx->type.btype, p_val);
    }

    return nullptr;
}

OpToLoop LoopGen::op_to_loop(Op& op)
{
    OpToLoop otl;

    // Only support single indexed operations
    ASSERT(op.idxs.size() == 1);

    // Loop index allocation
    auto loop_idx_addr_expr = make_shared<Alloc>(types::IDX);
    otl.loop_idx_addr =
        make_shared<SymNode>("loop_idx_addr", loop_idx_addr_expr);
    this->assign(otl.loop_idx_addr, loop_idx_addr_expr);
    auto load_loop_idx_expr = make_shared<Load>(otl.loop_idx_addr);

    // Map op idx to loop idx
    otl.op_idx = make_shared<SymNode>(op.idxs[0]->name, op.idxs[0]);
    this->map_sym(op.idxs[0], otl.op_idx);
    auto loop_idx_to_op_idx_expr =
        make_shared<Cast>(otl.op_idx->type, load_loop_idx_expr);
    this->assign(otl.op_idx, loop_idx_to_op_idx_expr);

    // Loop init statement
    otl.init = make_shared<Store>(
        otl.loop_idx_addr,
        make_shared<Cast>(types::IDX, get_init_val(op.idxs[0], op.pred)));

    // Loop exit condition
    otl.exit_cond = make_shared<GreaterThan>(
        make_shared<Load>(otl.loop_idx_addr),
        make_shared<Cast>(types::IDX, get_exit_val(op.idxs[0], op.pred)));

    // Loop index increment expression
    otl.incr = make_shared<Store>(
        otl.loop_idx_addr,
        make_shared<Add>(load_loop_idx_expr,
                         make_shared<Const>(BaseType::IDX, 1)));

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
    red_loop->body_cond = nullptr;
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
