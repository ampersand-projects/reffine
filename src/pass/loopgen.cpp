#include "reffine/pass/loopgen.h"

using namespace std;
using namespace reffine;

Expr LoopGen::visit(Reduce& red)
{
    // Index allocation and initialization
    auto idx_addr = make_shared<Alloc>(types::IDX);
    auto idx_addr_sym = make_shared<SymNode>("idx_addr", idx_addr);
    map_val(idx_addr_sym, idx_addr);
    auto idx_init_stmt = make_shared<Store>(idx_addr_sym, make_shared<Const>(BaseType::IDX, 0));

    // State allocation and initialization
    auto state_addr = make_shared<Alloc>(red.type);
    auto state_addr_sym = make_shared<SymNode>("state_addr", state_addr);
    map_val(state_addr_sym, state_addr);
    auto state_init_stmt = make_shared<Store>(state_addr_sym, red.init());

    // Loop increment
    auto new_idx = make_shared<Add>(
        make_shared<Load>(idx_addr_sym),
        make_shared<Const>(BaseType::IDX, 1)
    );
    auto incr_stmt = make_shared<Store>(idx_addr_sym, new_idx);

    // Loop exit condition expression
    auto exit_cond_expr = make_shared<GreaterThanEqual>(
        make_shared<Load>(idx_addr_sym),
        make_shared<Const>(BaseType::IDX, 10)
    );

    // Loop body statement
    auto idx_val = make_shared<Cast>(types::INT64, make_shared<Load>(idx_addr_sym));
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
