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
