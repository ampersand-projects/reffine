#include "reffine/pass/loopgen.h"

#include "reffine/pass/z3solver.h"

using namespace std;
using namespace reffine;

Expr get_init_val(Sym idx, vector<Expr> preds)
{
    for (auto pred : preds) {
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
    }

    return nullptr;
}

Expr get_exit_val(Sym idx, vector<Expr> preds)
{
    for (auto pred : preds) {
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
    }

    return nullptr;
}

OpToLoop LoopGen::op_to_loop(Op& op)
{
    OpToLoop otl;
    otl.op_idx = op.idxs[0];

    // Loop index allocation
    auto idx_addr = make_shared<Alloc>(otl.op_idx->type);
    otl.loop_idx_addr = make_shared<SymNode>("idx_addr", idx_addr);
    map_val(otl.loop_idx_addr, idx_addr);

    // Loop index bound expressions
    otl.lower_bound = get_init_val(otl.op_idx, op.preds);
    otl.upper_bound = get_exit_val(otl.op_idx, op.preds);

    // Loop index increment expression
    otl.next_loop_idx = make_shared<Add>(
        make_shared<Load>(otl.loop_idx_addr),
        make_shared<Const>(otl.op_idx->type.btype, 1)
    );

    return otl;
}

Expr LoopGen::visit(Reduce& red)
{
    auto otl = op_to_loop(red.op);

    // State allocation and initialization
    auto state_addr_expr = make_shared<Alloc>(red.type);
    auto state_addr = make_shared<SymNode>("state_addr", state_addr_expr);
    map_val(state_addr, state_addr_expr);

    // Loop body statement
    auto val = make_shared<New>(vector<Expr>{make_shared<Load>(otl.loop_idx_addr)});
    auto state = make_shared<Load>(state_addr);
    auto new_state = red.acc(state, val);
    auto body_stmt = make_shared<Store>(state_addr, new_state);

    // Loop definition
    auto red_loop = make_shared<Loop>(make_shared<Load>(state_addr));
    red_loop->init = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(otl.loop_idx_addr, otl.lower_bound),
        make_shared<Store>(state_addr, red.init()),
    });
    red_loop->incr = make_shared<Store>(otl.loop_idx_addr, otl.next_loop_idx);
    red_loop->exit_cond = make_shared<GreaterThan>(make_shared<Load>(otl.loop_idx_addr), otl.upper_bound);
    red_loop->body_cond = nullptr;
    red_loop->body = body_stmt;
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
