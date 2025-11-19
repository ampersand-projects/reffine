#include "reffine/builder/reffiner.h"
#include "reffine/utils/utils.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

struct TPCHQuery6 {
    shared_ptr<ArrowTable2> tbl;
    void (*query6_fn)(double*, ArrowTable*);

    TPCHQuery6(int64_t start, int64_t end, double disc, double quant)
    {
        this->tbl = load_arrow_file("../benchmark/lib/lineitem.arrow", 2);
        this->query6_fn = compile_op<void (*)(double*, ArrowTable*)>(
            this->build_op(tbl, start, end, disc, quant));
    }

    shared_ptr<Func> build_op(shared_ptr<ArrowTable2> tbl, int64_t start,
                              int64_t end, double disc, double quant)
    {
        auto vec_in_sym = _sym("vec_in", tbl->get_data_type());
        auto red = _red(
            _subvec(vec_in_sym, _idx(0), _len(vec_in_sym, 1)),
            []() { return _f64(0); },
            [start, end, disc, quant](Expr s, Expr v) {
                auto start_date = _i64(start);
                auto end_date = _i64(end);
                auto discount = _f64(disc);
                auto quantity = _f64(quant);

                auto l_quantity = _get(v, 3);
                auto l_extendedprice = _get(v, 4);
                auto l_discount = _get(v, 5);
                auto l_shipdate = _get(v, 9);

                auto date_pred =
                    _gte(l_shipdate, start_date) & _lt(l_shipdate, end_date);
                auto discount_pred =
                    _gte(l_discount, _sub(discount, _f64(0.01))) &
                    _lte(l_discount, _add(discount, _f64(0.01)));
                auto quantity_pred = _lt(l_quantity, quantity);
                auto pred = date_pred & discount_pred & quantity_pred;
                auto new_s = _add(s, _mul(l_extendedprice, l_discount));
                return _sel(pred, new_s, s);
            });
        auto red_sym = _sym("red", red);

        auto fn = _func("query6", red_sym, vector<Sym>{vec_in_sym});
        fn->tbl[red_sym] = red;

        return fn;
    }

    double run()
    {
        double out;
        this->query6_fn(&out, this->tbl.get());
        return out;
    }
};
