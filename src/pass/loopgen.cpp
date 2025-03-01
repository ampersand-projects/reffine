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

shared_ptr<Loop> LoopGen::build_loop(Op& op)
{
    ASSERT(op.iters.size() == 1);
    ASSERT(op._lower != nullptr);
    ASSERT(op._upper != nullptr);
    ASSERT(op._incr != nullptr);

    // Loop index allocation
    auto idx_alloc = _alloc(_idx_t);
    auto idx_addr = _sym(op.iters[0]->name + "_addr", idx_alloc);
    this->assign(idx_addr, idx_alloc);

    // Map op idx to loop idx
    auto idx_load = _load(idx_addr);
    auto idx = _sym(op.iters[0]->name, idx_load);
    this->map_sym(op.iters[0], idx);
    this->assign(idx, idx_load);

    // Loop bound and increment
    auto lb_expr = eval(op._lower);
    auto ub_expr = eval(op._upper);
    auto incr_expr = eval(op._incr);
    auto cond_expr = eval(op.pred);

    // Loop output
    vector<Expr> outputs;
    for (auto output : op.outputs) { outputs.push_back(eval(output)); }

    // Loop definition
    auto loop = _loop(_new(outputs));
    loop->init = _store(idx_addr, lb_expr);
    loop->incr = _store(idx_addr, incr_expr);
    loop->exit_cond = _gt(idx, ub_expr);
    loop->body_cond = nullptr;
    loop->body = nullptr;
    loop->post = nullptr;

    return std::move(loop);
}

Expr LoopGen::visit(Reduce& red)
{
    auto red_loop = this->build_loop(red.op);

    // State allocation and initialization
    auto state_addr_expr = _alloc(red.type);
    auto state_addr = _sym("state_addr", state_addr_expr);
    this->assign(state_addr, state_addr_expr);
    auto load_state_expr = _load(state_addr);

    // Finalize loop
    red_loop->init = _stmts(vector<Stmt>{
        red_loop->init,
        _store(state_addr, red.init()),
    });
    red_loop->body =
        _store(state_addr, red.acc(load_state_expr, red_loop->output));
    red_loop->output = load_state_expr;

    return red_loop;
}

shared_ptr<Func> LoopGen::Build(shared_ptr<Func> op_func)
{
    auto loop_func = _func(op_func->name, nullptr, vector<Sym>{});

    LoopGenCtx ctx(op_func, loop_func);
    LoopGen loopgen(ctx);
    op_func->Accept(loopgen);

    return std::move(loop_func);
}
