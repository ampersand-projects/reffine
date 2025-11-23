#include "arrow/acero/exec_plan.h"
#include "arrow/acero/options.h"
#include "arrow/compute/api_aggregate.h"
#include "arrow/result.h"
#include "arrow/table.h"
#include "arrow/util/bit_util.h"
#include "arrow/util/string.h"
#include "reffine/builder/reffiner.h"
#include "reffine/utils/utils.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

struct AlgoTrading {
    using QueryFnTy = void (*)(ArrowTable**, ArrowTable*, ArrowTable*);

    shared_ptr<ArrowTable2> stock_price;
    QueryFnTy query_fn;

    AlgoTrading()
    {
        this->stock_price =
            load_arrow_file("../benchmark/arrow_data/stock_price.arrow", 1);
        this->stock_price->build_index();
        this->query_fn = compile_op<QueryFnTy>(this->build_op());
    }

    shared_ptr<Func> build_op()
    {
        auto stock_price1 = _sym("stock_price1", this->stock_price->get_data_type());
        auto stock_price2 = _sym("stock_price2", this->stock_price->get_data_type());
        auto t_sym = _sym("t", _i64_t);
        auto t50 = _add(t_sym, _i64(-50));
        auto t50_sym = _sym("t50", t50);

        auto cur_val = _get(stock_price1[t_sym], 0);
        auto tail_val = _get(stock_price2[t50_sym], 0);
        auto diff = _sub(cur_val, tail_val);
        auto diff_sym = _sym("diff", diff);
        auto op = _op(vector<Sym>{t_sym},
                _in(t_sym, stock_price1) | _in(t50_sym, stock_price2),
                vector<Expr>{diff_sym}
        );
        auto op_sym = _sym("op", op);

        auto fn = _func("algotrading", op_sym, vector<Sym>{stock_price1, stock_price2});
        fn->tbl[op_sym] = op;
        fn->tbl[t50_sym] = t50;
        fn->tbl[diff_sym] = diff;

        return fn;
    }

    ArrowTable* run()
    {
        ArrowTable* out;
        this->query_fn(&out, this->stock_price.get(), this->stock_price.get());
        return out;
    }
};

struct Nbody {
    using QueryFnTy = void (*)(ArrowTable**, ArrowTable*);

    shared_ptr<ArrowTable2> bodies;
    QueryFnTy query_fn;

    Nbody()
    {
        this->bodies =
            load_arrow_file("../benchmark/arrow_data/bodies.arrow", 1);
        this->bodies->build_index();
        this->query_fn = compile_op<QueryFnTy>(this->build_op(2048, 1.0, 0.01));
    }

    shared_ptr<Func> build_op(int64_t N, double G, double dt)
    {
        auto bodies = _sym("bodies", this->bodies->get_data_type());
        auto id_a = _sym("id_a", _idx_t);
        auto id_b = _sym("id_b", _idx_t);

        auto cond = _not(_eq(id_a, id_b));
        auto cond_sym = _sym("cond", cond);

        auto xa = _readdata(bodies, id_a, 2);
        auto xb = _readdata(bodies, id_b, 2);
        auto dx = _sub(xa, xb);
        auto dx_sym = _sym("dx", dx);
        auto ya = _readdata(bodies, id_a, 3);
        auto yb = _readdata(bodies, id_b, 3);
        auto dy = _sub(ya, yb);
        auto dy_sym = _sym("dy", dy);
        auto za = _readdata(bodies, id_a, 4);
        auto zb = _readdata(bodies, id_b, 4);
        auto dz = _sub(za, zb);
        auto dz_sym = _sym("dz", dz);
        auto dist2 =
            _mul(dx_sym, dx_sym) + _mul(dy_sym, dy_sym) + _mul(dz_sym, dz_sym);
        auto dist2_sym = _sym("dist2", dist2);
        auto dist = _sqrt(dist2_sym);
        auto dist_sym = _sym("dist", dist);

        auto ma = _readdata(bodies, id_a, 1);
        auto mb = _readdata(bodies, id_b, 1);
        auto f = _div(_mul(_f64(G), _mul(ma, mb)), dist2_sym);
        auto f_sym = _sym("f", f);
        auto fx = _div(_mul(f_sym, dx_sym), dist_sym);
        auto fx_sym = _sym("fx", fx);
        auto fy = _div(_mul(f_sym, dy_sym), dist_sym);
        auto fy_sym = _sym("fy", fy);
        auto fz = _div(_mul(f_sym, dz_sym), dist_sym);
        auto fz_sym = _sym("fz", fz);

        auto op = _op(vector<Sym>{id_a, id_b},
                      cond_sym & _gte(id_a, _idx(0)) & _lt(id_a, _idx(N)) &
                          _gte(id_b, _idx(0)) & _lt(id_b, _idx(N)),
                      vector<Expr>{fx_sym, fy_sym, fz_sym});
        auto op_sym = _sym("pairs", op);

        auto idx = _sym("idx", _idx_t);
        auto sfx = _red(
            _subvec(op_sym, _mul(idx, _idx(N)),
                    _mul(_add(idx, _idx(1)), _idx(N))),
            []() { return _f64(0); },
            [](Expr s, Expr v) { return _add(s, _get(v, 1)); });
        auto sfx_sym = _sym("sfx", sfx);
        auto sfy = _red(
            _subvec(op_sym, _mul(idx, _idx(N)),
                    _mul(_add(idx, _idx(1)), _idx(N))),
            []() { return _f64(0); },
            [](Expr s, Expr v) { return _add(s, _get(v, 2)); });
        auto sfy_sym = _sym("sfy", sfy);
        auto sfz = _red(
            _subvec(op_sym, _mul(idx, _idx(N)),
                    _mul(_add(idx, _idx(1)), _idx(N))),
            []() { return _f64(0); },
            [](Expr s, Expr v) { return _add(s, _get(v, 3)); });
        auto sfz_sym = _sym("sfz", sfz);
        auto op2 = _op(vector<Sym>{idx}, _gte(idx, _idx(0)) & _lt(idx, _idx(N)),
                       vector<Expr>{sfx_sym, sfy_sym, sfz_sym});
        auto op2_2 = _initval(vector<Sym>{op_sym}, op2);
        auto op2_sym = _sym("force", op2_2);

        auto id = _sym("ID", _idx_t);
        auto old_m = _readdata(bodies, id, 1);
        auto old_x = _readdata(bodies, id, 2);
        auto old_y = _readdata(bodies, id, 3);
        auto old_z = _readdata(bodies, id, 4);
        auto old_vx = _readdata(bodies, id, 5);
        auto old_vy = _readdata(bodies, id, 6);
        auto old_vz = _readdata(bodies, id, 7);
        auto new_m_sym = _sym("M", old_m);
        auto new_vx_expr = _add(
            old_vx, _mul(_div(_readdata(op2_sym, id, 1), new_m_sym), _f64(dt)));
        auto new_vx_sym = _sym("VX", new_vx_expr);
        auto new_vy_expr = _add(
            old_vy, _mul(_div(_readdata(op2_sym, id, 2), new_m_sym), _f64(dt)));
        auto new_vy_sym = _sym("VY", new_vy_expr);
        auto new_vz_expr = _add(
            old_vz, _mul(_div(_readdata(op2_sym, id, 3), new_m_sym), _f64(dt)));
        auto new_vz_sym = _sym("VZ", new_vz_expr);
        auto new_x = _add(old_x, _mul(new_vx_sym, _f64(dt)));
        auto new_x_sym = _sym("X", new_x);
        auto new_y = _add(old_y, _mul(new_vy_sym, _f64(dt)));
        auto new_y_sym = _sym("Y", new_y);
        auto new_z = _add(old_z, _mul(new_vz_sym, _f64(dt)));
        auto new_z_sym = _sym("Z", new_z);

        auto op3 = _op(vector<Sym>{id}, _gte(id, _idx(0)) & _lt(id, _idx(N)),
                       vector<Expr>{new_m_sym, new_x_sym, new_y_sym, new_z_sym,
                                    new_vx_sym, new_vy_sym, new_vz_sym});
        auto op3_2 = _initval(vector<Sym>{op2_sym}, op3);
        auto op3_sym = _sym("new_bodies", op3_2);

        auto fn = _func("nbodies", op3_sym, vector<Sym>{bodies});
        fn->tbl[new_m_sym] = old_m;
        fn->tbl[new_vx_sym] = new_vx_expr;
        fn->tbl[new_vy_sym] = new_vy_expr;
        fn->tbl[new_vz_sym] = new_vz_expr;
        fn->tbl[new_x_sym] = new_x;
        fn->tbl[new_y_sym] = new_y;
        fn->tbl[new_z_sym] = new_z;
        fn->tbl[cond_sym] = cond;
        fn->tbl[dx_sym] = dx;
        fn->tbl[dy_sym] = dy;
        fn->tbl[dz_sym] = dz;
        fn->tbl[dist2_sym] = dist2;
        fn->tbl[dist_sym] = dist;
        fn->tbl[f_sym] = f;
        fn->tbl[fx_sym] = fx;
        fn->tbl[fy_sym] = fy;
        fn->tbl[fz_sym] = fz;
        fn->tbl[op_sym] = op;
        fn->tbl[op2_sym] = op2_2;
        fn->tbl[sfx_sym] = sfx;
        fn->tbl[sfy_sym] = sfy;
        fn->tbl[sfz_sym] = sfz;
        fn->tbl[op3_sym] = op3_2;

        return fn;
    }

    ArrowTable* run()
    {
        ArrowTable* out;
        this->query_fn(&out, this->bodies.get());
        return out;
    }
};

struct PageRank {
    using QueryFnTy = void (*)(ArrowTable**, ArrowTable*, ArrowTable*,
                               ArrowTable*);

    shared_ptr<ArrowTable2> edges;
    shared_ptr<ArrowTable2> rev_edges;
    shared_ptr<ArrowTable2> pr;
    int64_t N;
    QueryFnTy query_fn;

    PageRank()
    {
        this->edges = load_arrow_file("../benchmark/arrow_data/edges.arrow", 2);
        this->rev_edges =
            load_arrow_file("../benchmark/arrow_data/rev_edges.arrow", 2);
        this->pr = load_arrow_file("../benchmark/arrow_data/pr.arrow", 1);
        this->N = 81306;
        this->edges->build_index();
        this->rev_edges->build_index();
        this->pr->build_index();
        this->query_fn = compile_op<QueryFnTy>(this->build_op(this->N, 0.85));
    }

    shared_ptr<Func> build_op(int64_t N, double alpha)
    {
        auto edges = _sym("edges", this->edges->get_data_type());
        auto rev_edges = _sym("rev_edges", this->rev_edges->get_data_type());
        auto pr = _sym("pr", this->pr->get_data_type());

        auto src = _sym("src", _i64_t);
        auto deg = _red(
            edges[src], []() { return _i64(0); },
            [](Expr s, Expr v) { return _add(s, _get(v, 1)); });
        auto deg_sym = _sym("deg", deg);
        auto contrib =
            _div(_cast(_f32_t, _get(pr[src], 0)), _cast(_f32_t, deg_sym));
        auto contrib_sym = _sym("contrib", contrib);
        auto outdeg = _op(vector<Sym>{src}, _in(src, edges) & _in(src, pr),
                          vector<Expr>{contrib_sym});
        auto outdeg_sym = _sym("outdeg", outdeg);
        auto outdeg2 = _buildidx(outdeg_sym);
        auto outdeg2_sym = _sym("outdeg2", outdeg2);

        auto dst = _sym("dst", _i64_t);
        auto dst_pr = _red(
            rev_edges[dst], []() { return _f32(0); },
            [outdeg2_sym](Expr s, Expr v) {
                auto src = _get(v, 0);
                auto count = _get(v, 1);
                return _add(
                    s, _mul(_cast(_f32_t, count), _get(outdeg2_sym[src], 0)));
            });
        auto dst_pr_sym = _sym("dst_pr", dst_pr);
        auto new_pr_val =
            _add(_mul(_f32(alpha), dst_pr_sym), _f32((1 - alpha) / N));
        auto new_pr_val_sym = _sym("new_pr_val", new_pr_val);
        auto new_pr_op = _op(vector<Sym>{dst}, _in(dst, rev_edges),
                             vector<Expr>{new_pr_val_sym});
        auto new_pr = _initval(vector<Sym>{outdeg2_sym}, new_pr_op);
        auto new_pr_sym = _sym("new_pr", new_pr);

        auto fn =
            _func("pagerank", new_pr_sym, vector<Sym>{edges, rev_edges, pr});
        fn->tbl[outdeg_sym] = outdeg;
        fn->tbl[outdeg2_sym] = outdeg2;
        fn->tbl[deg_sym] = deg;
        fn->tbl[contrib_sym] = contrib;
        fn->tbl[new_pr_sym] = new_pr;
        fn->tbl[dst_pr_sym] = dst_pr;
        fn->tbl[new_pr_val_sym] = new_pr_val;

        return fn;
    }

    ArrowTable* run()
    {
        ArrowTable* contrib;
        this->query_fn(&contrib, this->edges.get(), this->rev_edges.get(),
                       this->pr.get());
        return contrib;
    }
};
