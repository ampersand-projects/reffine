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
    auto iter = eval(elem.iter);
    auto vec = eval(elem.vec);

    Expr idx;
    if (this->_vec_iter_idx_map.contains(elem.vec)) {
        idx = this->_vec_iter_idx_map.at(elem.vec).at(elem.iter);
    } else {
        idx = _locate(vec, iter);
    }

    switch (vec->type.dim) {
        case 1: {
            vector<Expr> vals;
            for (size_t i = vec->type.dim; i < vec->type.dtypes.size(); i++) {
                auto data = _readdata(vec, idx, i);
                vals.push_back(data);
            }

            return _new(vals);
        } 
        case 2: {
            auto start_idx = _sel(_lte(idx, _idx(0)), _idx(0), _readrunend(vec, _sub(idx, _idx(1)), 0));
            auto end_idx = _readrunend(vec, idx, 0);
            return _subvec(vec, start_idx, end_idx);
        }
        default: 
            throw runtime_error("Only support 1d and 2d vectors");
    }
}

shared_ptr<Loop> LoopGen::build_loop(Op& op, shared_ptr<Loop> loop)
{
    Reffine rpass(make_unique<ReffineCtx>(this->ctx().in_sym_tbl));
    for (auto input : this->ctx().out_func->inputs) {
        if (input->type.is_val()) { rpass.vars().insert(input); }
    }
    auto ispace = rpass.eval(this->tmp_expr(op));

    vector<Expr> loop_inits;

    string loop_iter_name = "";
    for (auto iter : op.iters) { loop_iter_name += ("_" + iter->name); }

    // Loop lower bound
    auto lb = eval(ispace->lower_bound());
    auto lb_sym = lb->symify("lb");
    this->assign(lb_sym, lb);
    this->map_sym(lb_sym, lb_sym);

    // Loop index initialization
    auto idx_init = eval(ispace->iter_to_idx(lb_sym));
    auto idx_alloc = _alloc(idx_init->type);
    auto idx_addr = _sym(loop_iter_name + "_idx_addr", idx_alloc);
    this->assign(idx_addr, idx_alloc);
    this->map_sym(idx_addr, idx_addr);
    loop_inits.push_back(_store(idx_addr, idx_init));

    // Populate iter_elem_map
    // Used for lowering Element expressions
    for (auto& [vec, iter, idx] : ispace->vec_iter_idxs(_load(idx_addr))) {
        this->_vec_iter_idx_map[vec][iter] = idx;
    }

    // Populate additional symbols
    for (auto& [extra_sym, expr] : ispace->extra_syms()) {
        this->assign(extra_sym, eval(expr));
        this->map_sym(extra_sym, extra_sym);
        loop_inits.push_back(extra_sym);
    }

    // Derive op iterator from loop idx
    auto loop_iter_expr = eval(ispace->idx_to_iter(_load(idx_addr)));
    auto loop_iter = _sym(loop_iter_name, loop_iter_expr);
    this->assign(loop_iter, loop_iter_expr);

    // Map op iters to loop iter
    for (size_t i = 0; i < op.iters.size(); i++) {
        auto iter = op.iters[i];
        auto new_iter = _sym(iter->name, iter);
        this->assign(new_iter, _get(loop_iter, i));
        this->map_sym(iter, new_iter);
    }

    // Loop output
    vector<Expr> outputs;
    for (auto iter : op.iters) { outputs.push_back(eval(iter)); }
    for (auto output : op.outputs) { outputs.push_back(eval(output)); }

    // Loop definition
    loop->init = _stmts(loop_inits);
    loop->incr = _stmts(
        vector<Expr>{_store(idx_addr, eval(ispace->next(_load(idx_addr))))});
    loop->exit_cond = _not(eval(ispace->is_alive(_load(idx_addr))));
    loop->body_cond = eval(ispace->iter_cond(_load(idx_addr)));
    loop->output = _new(outputs);

    return loop;
}

Expr LoopGen::visit(Op& op)
{
    auto len = _idx(1000000);

    // Output vector builder
    auto mem_id = memman.add_builder([op](int64_t len) {
        vector<DataType> out_dtypes;
        vector<string> out_cols;

        for (auto i : op.iters) {
            out_dtypes.push_back(i->type);
            out_cols.push_back(i->str());
        }

        for (auto o : op.outputs) {
            out_dtypes.push_back(o->type);
            out_cols.push_back(o->str());
        }

        return make_shared<ArrowTable2>("out", op.iters.size(), len, out_cols,
                                        out_dtypes);
    });
    auto out_vec = _make(op.type, len, mem_id);
    auto out_vec_sym = _sym("out_vec", out_vec);
    this->assign(out_vec_sym, out_vec);

    // Build loop
    auto loop = this->build_loop(op, _loop(out_vec_sym));

    // Output vector index
    auto out_vec_idx_alloc = _alloc(_idx_t);
    auto out_vec_idx_addr = _sym("out_vec_idx_addr", out_vec_idx_alloc);
    this->assign(out_vec_idx_addr, out_vec_idx_alloc);

    // Counter for the number of nulls in the output
    auto null_count_alloc = _alloc(_idx_t);
    auto null_count_addr = _sym("null_count", null_count_alloc);
    this->assign(null_count_addr, null_count_alloc);

    // Loop body condition
    auto body_cond_sym = _sym("body_cond", loop->body_cond);
    this->assign(body_cond_sym, loop->body_cond);
    loop->body_cond = body_cond_sym;

    // Temporary boolean array for storing bitmap
    // Used only for vectorization
    auto bytemap = _alloc(types::BOOL, len);
    auto bytemap_sym = _sym("bytemap", bytemap);
    this->assign(bytemap_sym, bytemap);

    // Write the output to the out_vec
    vector<Expr> body_stmts;
    for (size_t i = 0; i < op.iters.size() + op.outputs.size(); i++) {
        body_stmts.push_back(_writedata(out_vec_sym, _load(out_vec_idx_addr), i,
                                        _get(loop->output, i)));
    }

    // Update loop
    loop->init = _stmts(vector<Expr>{
        loop->init,
        _store(out_vec_idx_addr, _idx(0)),
        _store(null_count_addr, _idx(0)),
        out_vec_sym,
        bytemap_sym,
    });
    loop->body = _stmts(body_stmts);

    if (this->_vectorize) {
        loop->body = _stmts(vector<Expr>{
            loop->body,
            _store(bytemap_sym, loop->body_cond, _load(out_vec_idx_addr)),
            _store(null_count_addr,
                   _add(_load(null_count_addr),
                        _sel(loop->body_cond, _idx(0), _idx(1)))),
        });
        loop->body_cond = nullptr;
    }

    loop->body = _stmts(vector<Expr>{
        loop->body,
        _store(out_vec_idx_addr, _add(_load(out_vec_idx_addr), _idx(1))),
    });
    loop->post = _finalize(out_vec_sym, bytemap_sym, _load(out_vec_idx_addr),
                           _load(null_count_addr));
    loop->output = out_vec_sym;
    auto loop_sym = _sym("loop", loop);
    this->assign(loop_sym, loop);

    return loop_sym;
}

Expr LoopGen::visit(Reduce& red)
{
    // Only allow aggregating unidimensional vectors
    ASSERT(red.vec->type.dim == 1);

    // State allocation and initialization
    auto state_alloc = _alloc(red.type);
    auto state_addr = _sym("state_addr", state_alloc);
    this->assign(state_addr, state_alloc);

    // Build reduction loop
    shared_ptr<Loop> loop;
    if (auto op_ptr = dynamic_pointer_cast<Op>(red.vec)) {
        loop = this->build_loop(*op_ptr, _loop(state_addr));
    } else if (auto subvec_ptr = dynamic_pointer_cast<SubVector>(eval(red.vec))) {
        auto red_vec = subvec_ptr->vec;
        auto red_start = subvec_ptr->start;
        auto red_end = subvec_ptr->end;

        auto red_idx_alloc = _alloc(red_start->type);
        auto red_idx_addr = _sym("red_idx_addr", red_idx_alloc);
        this->assign(red_idx_addr, red_idx_alloc);
        this->map_sym(red_idx_addr, red_idx_addr);

        vector<Expr> red_outputs;
        for (size_t i = 1; i < red_vec->type.dtypes.size(); i++) {
            red_outputs.push_back(_readdata(red_vec, _load(red_idx_addr), i));
        }
        loop = _loop(state_addr);
        loop->init = _stmts(vector<Expr>{
            _store(red_idx_addr, red_start)
        });
        loop->incr = _stmts(vector<Expr>{
            _store(red_idx_addr, _add(_load(red_idx_addr), _idx(1)))
        });
        loop->exit_cond = _gte(_load(red_idx_addr), red_end);
        loop->output = _new(red_outputs);
    } else {
        throw runtime_error("Unsupported reduction vector");
    }

    // Build reduce loop
    loop->init = _stmts(vector<Expr>{
        loop->init,
        _store(state_addr, red.init()),
    });

    if (this->_vectorize) {
        loop->body = _stmts(vector<Expr>{
            _store(state_addr, _sel(loop->body_cond,
                                    red.acc(_load(state_addr), loop->output),
                                    _load(state_addr)))
            });
        loop->body_cond = nullptr;
    } else {
        loop->body = _stmts(vector<Expr>{
            _store(state_addr, red.acc(_load(state_addr), loop->output))
        });
    }

    loop->output = state_addr;
    auto loop_sym = _sym("red_loop", loop);
    this->assign(loop_sym, loop);

    return _load(loop_sym);
}
