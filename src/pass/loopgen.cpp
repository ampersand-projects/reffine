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

Expr OpToLoop::visit(Op& op)
{
    // Index allocation
    this->op_idx = op.idxs[0];
    auto loop_idx_addr = make_shared<Alloc>(this->op_idx->type);
    this->loop_idx = make_shared<SymNode>("idx_addr", loop_idx_addr);
    map_val(this->loop_idx, loop_idx_addr);

    return nullptr;
}

Expr LoopGen::visit(Reduce& red)
{
    OpToLoop op_to_loop(ctx());
    red.op.Accept(op_to_loop);

    // State allocation and initialization
    auto state_addr = make_shared<Alloc>(red.type);
    auto state_addr_sym = make_shared<SymNode>("state_addr", state_addr);
    map_val(state_addr_sym, state_addr);
    auto state_init_stmt = make_shared<Store>(state_addr_sym, red.init());

    // Index allocation
    auto idx = red.op.idxs[0];
    auto idx_addr = make_shared<Alloc>(idx->type);
    auto idx_addr_sym = make_shared<SymNode>("idx_addr", idx_addr);
    map_val(idx_addr_sym, idx_addr);

    // Index initialization
    auto idx_init_val = get_init_val(idx, red.op.preds);
    auto idx_init_stmt = make_shared<Store>(idx_addr_sym, idx_init_val);

    // Loop increment
    auto new_idx = make_shared<Add>(make_shared<Load>(idx_addr_sym),
                                    make_shared<Const>(idx->type.btype, 1));
    auto incr_stmt = make_shared<Store>(idx_addr_sym, new_idx);

    // Loop exit condition expression
    auto exit_val = get_exit_val(idx, red.op.preds);
    auto exit_cond_expr =
        make_shared<GreaterThan>(make_shared<Load>(idx_addr_sym), exit_val);

    // Loop body statement
    auto idx_val =
        make_shared<Cast>(types::INT64, make_shared<Load>(idx_addr_sym));
    auto val = make_shared<New>(vector<Expr>{idx_val});
    auto state = make_shared<Load>(state_addr_sym);
    auto new_state = red.acc(state, val);
    auto body_stmt = make_shared<Store>(state_addr_sym, new_state);

    // Loop definition
    auto red_loop = make_shared<Loop>(make_shared<Load>(state_addr_sym));
    red_loop->init = make_shared<Stmts>(vector<Stmt>{
        idx_init_stmt,
        state_init_stmt,
    });
    red_loop->incr = incr_stmt;
    red_loop->exit_cond = exit_cond_expr;
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
