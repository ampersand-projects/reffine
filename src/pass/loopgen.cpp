#include "reffine/pass/loopgen.h"

#include "reffine/builder/reffiner.h"
#include "reffine/engine/memory.h"
#include "reffine/pass/reffinepass.h"
#include "reffine/pass/z3solver.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

Expr LoopGen::visit(Element& elem)
{
    eval(elem.iters[0]);
    auto vec = eval(elem.vec);

    auto idx = this->_vec_iter_idx_map.at(elem.vec).at(elem.iters[0]);

    vector<Expr> vals;
    for (size_t i = vec->type.dim; i < vec->type.dtypes.size(); i++) {
        auto data = _load(_fetch(vec, i), idx);
        vals.push_back(data);
    }

    return _new(vals);
}

pair<shared_ptr<Loop>, vector<Expr>> LoopGen::build_loop(Op& op)
{
    // Only support 1d operators now
    ASSERT(op.iters.size() == 1);
    auto iter = op.iters[0];

    auto ispace = Reffine::Build(op, this->ctx().in_sym_tbl);

    vector<Expr> loop_inits;

    // Loop index initialization
    auto idx_init = eval(ispace->iter_to_idx(ispace->lower_bound()));
    auto idx_alloc = _alloc(idx_init->type);
    auto idx_addr = _sym(iter->name + "_idx_addr", idx_alloc);
    this->assign(idx_addr, idx_alloc);
    this->map_sym(idx_addr, idx_addr);
    loop_inits.push_back(_store(idx_addr, idx_init));

    // Populate iter_elem_map
    // Used for lowering Element expressions
    for (auto& [vec, idx] : ispace->vec_idxs(_load(idx_addr))) {
        this->_vec_iter_idx_map[vec][iter] = idx;
    }

    // Populate additional symbols
    for (auto& [extra_sym, expr] : ispace->extra_syms()) {
        this->assign(extra_sym, eval(expr));
        this->map_sym(extra_sym, extra_sym);
        loop_inits.push_back(extra_sym);
    }

    // Derive op iterator from loop idx
    auto loop_iter = _sym(iter->name, iter);
    auto iter_expr = eval(ispace->idx_to_iter(_load(idx_addr)));
    this->assign(loop_iter, iter_expr);
    this->map_sym(iter, loop_iter);

    // Loop output
    vector<Expr> outputs;
    outputs.push_back(loop_iter);
    for (auto output : op.outputs) { outputs.push_back(eval(output)); }

    // Loop definition
    auto loop = _loop(_new(outputs));
    loop->init = _stmts(loop_inits);
    loop->incr = _store(idx_addr, eval(ispace->next(_load(idx_addr))));
    loop->exit_cond = _not(eval(ispace->is_alive(_load(idx_addr))));
    loop->body_cond = eval(ispace->iter_cond(_load(idx_addr)));

    return {loop, outputs};
}

Expr LoopGen::visit(Op& op)
{
    auto len = _idx(100000);

    auto [tmp_loop, outputs] = this->build_loop(op);

    // Output vector builder
    auto mem_id = memman.add_builder([outputs](int64_t len) {
        vector<DataType> out_dtypes;
        vector<string> out_cols;
        for (auto o : outputs) {
            out_dtypes.push_back(o->type);
            out_cols.push_back(o->str());
        }
        return make_shared<ArrowTable2>("out", len, out_cols, out_dtypes);
    });
    auto out_vec = _make(op.type, len, mem_id);
    auto out_vec_sym = _sym("out_vec", out_vec);
    this->assign(out_vec_sym, out_vec);

    // Output vector index
    auto out_vec_idx_alloc = _alloc(_idx_t);
    auto out_vec_idx_addr = _sym("out_vec_idx_addr", out_vec_idx_alloc);
    this->assign(out_vec_idx_addr, out_vec_idx_alloc);

    // Body condition
    auto body_cond_sym = _sym("body_cond", tmp_loop->body_cond);
    this->assign(body_cond_sym, tmp_loop->body_cond);

    // Temporary boolean array for bytemap
    // Helpful for vectorizing the loop
    auto bytemap = _alloc(types::BOOL, len);
    auto bytemap_sym = _sym("bytemap", bytemap);
    this->assign(bytemap_sym, bytemap);

    // Write the output to the out_vec
    vector<Expr> body_stmts;
    for (size_t i = 0; i < outputs.size(); i++) {
        auto vec_ptr = _fetch(out_vec_sym, i);
        body_stmts.push_back(_store(vec_ptr, outputs[i], _load(out_vec_idx_addr)));
    }
    body_stmts.push_back(_store(bytemap_sym, body_cond_sym, _load(out_vec_idx_addr)));

    // Build loop
    auto loop = _loop(out_vec_sym);
    loop->init = _stmts(vector<Expr>{
        tmp_loop->init,
        _store(out_vec_idx_addr, _idx(0)),
        out_vec_sym,
        bytemap_sym,
    });
    loop->incr = _stmts(vector<Expr>{
        tmp_loop->incr,
        _store(out_vec_idx_addr, _add(_load(out_vec_idx_addr), _idx(1)))});
    loop->exit_cond = tmp_loop->exit_cond;
    loop->body = _stmts(body_stmts);
    loop->post = _stmts(vector<Expr>{
        _finalize(out_vec_sym, bytemap_sym, _load(out_vec_idx_addr)),
    });

    auto loop_sym = _sym("loop", loop);
    this->assign(loop_sym, loop);

    return loop_sym;
}

Expr LoopGen::visit(Reduce& red)
{
    auto [tmp_loop, _] = this->build_loop(red.op);

    // State allocation and initialization
    auto state_alloc = _alloc(red.type);
    auto state_addr = _sym("state_addr", state_alloc);
    this->assign(state_addr, state_alloc);

    // Output
    auto output =
        _sel(tmp_loop->body_cond, red.acc(_load(state_addr), tmp_loop->output),
             _load(state_addr));

    // Build reduce loop
    auto red_loop = _loop(state_addr);
    red_loop->init = _stmts(vector<Expr>{
        tmp_loop->init,
        _store(state_addr, red.init()),
    });
    red_loop->incr = tmp_loop->incr;
    red_loop->exit_cond = tmp_loop->exit_cond;
    red_loop->body_cond = nullptr;
    red_loop->body = _store(state_addr, output);

    auto red_loop_sym = _sym("red_loop", red_loop);
    this->assign(red_loop_sym, red_loop);

    return _load(red_loop_sym);
}
