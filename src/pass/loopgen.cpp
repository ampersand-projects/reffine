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

Expr LoopGen::visit(Lookup& lookup)
{
    auto vec = eval(lookup.vec);
    auto idx = eval(lookup.idx);

    vector<Expr> vals;
    for (size_t i = vec->type.dim; i < vec->type.dtypes.size(); i++) {
        auto data_ptr = _fetch(vec, idx, i);
        auto data = _load(data_ptr);
        vals.push_back(data);
    }

    return _new(vals);
}

shared_ptr<Loop> LoopGen::build_loop(Op& op)
{
    return nullptr;
}

Expr LoopGen::visit(Reduce& red)
{
    auto tmp_loop = this->build_loop(red.op);

    // State allocation and initialization
    auto state_addr_expr = _alloc(red.type);
    auto state_addr = _sym("state_addr", state_addr_expr);
    this->assign(state_addr, state_addr_expr);

    // Build reduce loop
    auto red_loop = _loop(state_addr);
    red_loop->init = _stmts(vector<Stmt>{
        tmp_loop->init,
        _store(state_addr, red.init()),
    });
    red_loop->incr = tmp_loop->incr;
    red_loop->exit_cond = tmp_loop->exit_cond;
    red_loop->body_cond = tmp_loop->body_cond;
    red_loop->body =
        _store(state_addr, red.acc(_load(state_addr), tmp_loop->output));
    red_loop->output = state_addr;

    auto red_loop_sym = _sym("red_loop", red_loop);
    this->assign(red_loop_sym, red_loop);

    return _load(red_loop_sym);
}

shared_ptr<Func> LoopGen::Build(shared_ptr<Func> op_func)
{
    auto loop_func = _func(op_func->name, nullptr, vector<Sym>{});

    LoopGenCtx ctx(op_func, loop_func);
    LoopGen loopgen(ctx);
    op_func->Accept(loopgen);

    return loop_func;
}
