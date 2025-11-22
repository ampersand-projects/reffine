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
        auto stock_price1 =
            _sym("stock_price1", this->stock_price->get_data_type());
        auto stock_price2 =
            _sym("stock_price2", this->stock_price->get_data_type());
        auto t_sym = _sym("t", _i64_t);
        auto i_sym = _cast(_idx_t, t_sym);

        auto short_win = _sub(i_sym, _idx(2));
        auto long_win = _sub(i_sym, _idx(4));

        auto current_read = _readdata(stock_price1, i_sym, 1);
        auto current_read_sym = _sym("current", current_read);
        auto short_read =
            _readdata(stock_price1,
                      _sel(_gte(short_win, _idx(0)), short_win, _idx(0)), 1);
        auto short_read_sym = _sym("shortwin", short_read);
        auto long_read = _readdata(
            stock_price1, _sel(_gte(long_win, _idx(0)), long_win, _idx(0)), 1);
        auto long_read_sym = _sym("longwin", long_read);
        auto op =
            _op(vector<Sym>{t_sym},
                _in(t_sym, stock_price1) | _in(t_sym, stock_price2),
                vector<Expr>{current_read_sym, short_read_sym - long_read_sym});
        auto op_sym = _sym("op", op);

        auto fn = _func("algotrading", op_sym,
                        vector<Sym>{stock_price1, stock_price2});
        fn->tbl[current_read_sym] = current_read;
        fn->tbl[short_read_sym] = short_read;
        fn->tbl[long_read_sym] = long_read;
        fn->tbl[op_sym] = op;

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
    using QueryFnTy = void (*)(ArrowTable**, ArrowTable*, ArrowTable*, int64_t);

    shared_ptr<ArrowTable2> edges;
    shared_ptr<ArrowTable2> pr;
    int64_t N;
    QueryFnTy query_fn;

    PageRank()
    {
        this->edges =
            load_arrow_file("../benchmark/arrow_data/edges.arrow", 2);
        this->pr =
            load_arrow_file("../benchmark/arrow_data/pr.arrow", 1);
        this->N = 81306;
        this->edges->build_index();
        this->pr->build_index();
        this->query_fn = compile_op<QueryFnTy>(this->build_op());
    }

    shared_ptr<Func> build_op()
    {
        auto edges = _sym("edges", this->edges->get_data_type());
        auto pr = _sym("pr", this->pr->get_data_type());
        auto N = _sym("N", _i64_t);

        auto n = _sym("src", _i64_t);

        auto deg = _red(edges[n],
            []() { return _i64(0); },
            [](Expr s, Expr v) { return _add(s, _get(v, 1)); }
        );
        auto deg_sym = _sym("deg", deg);
        auto outdeg = _op(vector<Sym>{n}, _in(n, edges),
            vector<Expr>{deg_sym}
        );
        auto outdeg_sym = _sym("outdeg", outdeg);

        auto fn = _func("pagerank", outdeg_sym, vector<Sym>{edges, pr, N});
        fn->tbl[outdeg_sym] = outdeg;
        fn->tbl[deg_sym] = deg;

        return fn;
    }

    ArrowTable* run()
    {
        ArrowTable* out;
        this->query_fn(&out, this->edges.get(), this->pr.get(), this->N);
        return out;
    }
};

