#include "reffine/pass/loopgen.h"

#include "reffine/builder/reffiner.h"
#include "reffine/pass/reffinepass.h"
#include "reffine/pass/z3solver.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

Expr LoopGen::visit(Element& elem)
{
    eval(elem.iters[0]);
    auto vec = eval(elem.vec);

    auto idx =
        this->_loopgenctx.vec_iter_idx_map.at(elem.vec).at(elem.iters[0]);

    vector<Expr> vals;
    for (size_t i = vec->type.dim; i < vec->type.dtypes.size(); i++) {
        auto col_ptr = _fetch_buf(vec, i);
        auto col_sym = _sym("col_" + std::to_string(i), col_ptr);
        this->assign(col_sym, col_ptr);

        auto data_ptr = _fetch(col_sym, idx);
        auto data = _load(data_ptr);
        vals.push_back(data);
    }

    return _new(vals);
}

shared_ptr<Loop> LoopGen::build_loop(Op& op)
{
    // Only support 1d operators now
    ASSERT(op.iters.size() == 1);
    auto iter = op.iters[0];

    auto ispace = Reffine::Build(op, this->ctx().in_sym_tbl);

    // Loop index initialization
    auto idx_init = eval(ispace->iter_to_idx(ispace->lower_bound()));
    auto idx_alloc = _alloc(idx_init->type);
    auto idx_addr = _sym(iter->name + "_idx_addr", idx_alloc);
    this->assign(idx_addr, idx_alloc);
    this->map_sym(idx_addr, idx_addr);

    // Popular iter_elem_map
    for (auto& [vec, idx] : ispace->vec_idxs(_load(idx_addr))) {
        this->_loopgenctx.vec_iter_idx_map[vec][iter] = idx;
    }

    // Derive op iterator from loop idx
    auto loop_iter = _sym(iter->name, iter);
    auto iter_expr = eval(ispace->idx_to_iter(_load(idx_addr)));
    this->assign(loop_iter, iter_expr);
    this->map_sym(iter, loop_iter);

    // Loop output
    vector<Expr> outputs;
    for (auto output : op.outputs) { outputs.push_back(eval(output)); }

    // Loop definition
    auto loop = _loop(_new(outputs));
    loop->init = _store(idx_addr, idx_init);
    loop->incr = _store(idx_addr, eval(ispace->advance(_load(idx_addr))));
    loop->exit_cond = _gt(loop_iter, eval(ispace->upper_bound()));
    auto cond = ispace->condition(_load(idx_addr));
    loop->body_cond = cond ? eval(cond) : nullptr;

    return loop;
}

Expr LoopGen::visit(Reduce& red)
{
    auto tmp_loop = this->build_loop(red.op);

    // State allocation and initialization
    auto state_alloc = _alloc(red.type);
    auto state_addr = _sym("state_addr", state_alloc);
    this->assign(state_addr, state_alloc);

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
