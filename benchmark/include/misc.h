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
        this->stock_price = load_arrow_file("../benchmark/arrow_data/stock_price.arrow", 1);
        this->stock_price->build_index();
        this->query_fn = compile_op<QueryFnTy>(this->build_op());
    }

    shared_ptr<Func> build_op()
    {
        auto stock_price1 = _sym("stock_price1", this->stock_price->get_data_type());
        auto stock_price2 = _sym("stock_price2", this->stock_price->get_data_type());
        auto t_sym = _sym("t", _i64_t);
        auto i_sym = _cast(_idx_t, t_sym);

        auto short_win = _sub(i_sym, _idx(2));
        auto long_win = _sub(i_sym, _idx(4));

        auto current_read = _readdata(stock_price1, i_sym, 1);
        auto current_read_sym = _sym("current", current_read);
        auto short_read = _readdata(stock_price1, _sel(_gte(short_win, _idx(0)), short_win, _idx(0)), 1);
        auto short_read_sym = _sym("shortwin", short_read);
        auto long_read = _readdata(stock_price1, _sel(_gte(long_win, _idx(0)), long_win, _idx(0)), 1);
        auto long_read_sym = _sym("longwin", long_read);
        auto op = _op(vector<Sym>{t_sym},
            _in(t_sym, stock_price1) | _in(t_sym, stock_price2),
            vector<Expr>{current_read_sym, short_read_sym - long_read_sym}
        );
        auto op_sym = _sym("op", op);

        auto fn = _func("algotrading", op_sym, vector<Sym>{stock_price1, stock_price2});
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
        this->bodies = load_arrow_file("../benchmark/arrow_data/bodies.arrow", 1);
        this->bodies->build_index();
        this->query_fn = compile_op<QueryFnTy>(this->build_op(3, 1.0));
    }

    shared_ptr<Func> build_op(int64_t N, double G)
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
        auto dist2 = _mul(dx_sym, dx_sym) + _mul(dy_sym, dy_sym);
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

        auto op = _op(vector<Sym>{id_a, id_b},
            cond_sym
            & _gte(id_a, _idx(0)) & _lt(id_a, _idx(N))
            & _gte(id_b, _idx(0)) & _lt(id_b, _idx(N)),
            vector<Expr>{fx_sym, fy_sym}
        );
        auto op_sym = _sym("pairs", op);

        auto fn = _func("nbodies", op_sym, vector<Sym>{bodies});
        fn->tbl[cond_sym] = cond;
        fn->tbl[dx_sym] = dx;
        fn->tbl[dy_sym] = dy;
        fn->tbl[dist2_sym] = dist2;
        fn->tbl[dist_sym] = dist;
        fn->tbl[f_sym] = f;
        fn->tbl[fx_sym] = fx;
        fn->tbl[fy_sym] = fy;
        fn->tbl[op_sym] = op;
        fn->tbl[cond_sym] = cond;

        return fn;
    }

    ArrowTable* run()
    {
        ArrowTable* out;
        this->query_fn(&out, this->bodies.get());
        return out;
    }
};


