#include "reffine/builder/reffiner.h"
#include "reffine/utils/utils.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

struct TPCHQuery3 {
    using QueryFnTy = void (*)(ArrowTable**, ArrowTable*, ArrowTable*,
                               ArrowTable*);

    shared_ptr<ArrowTable2> lineitem;
    shared_ptr<ArrowTable2> orders;
    shared_ptr<ArrowTable2> customer;
    QueryFnTy query_fn;

    TPCHQuery3()
    {
        this->lineitem =
            load_arrow_file("../benchmark/arrow_data/lineitem.arrow", 2);
        this->orders =
            load_arrow_file("../benchmark/arrow_data/orders.arrow", 1);
        this->customer =
            load_arrow_file("../benchmark/arrow_data/customer.arrow", 1);
        this->lineitem->build_index();
        this->orders->build_index();
        this->customer->build_index();
        this->query_fn = compile_op<QueryFnTy>(this->build_op(1, 795484800));
    }

    shared_ptr<Func> build_op(int8_t segment, int64_t date)
    {
        auto lineitem = _sym("lineitem", this->lineitem->get_data_type());
        auto orders = _sym("orders", this->orders->get_data_type());
        auto customer = _sym("customer", this->customer->get_data_type());
        auto orderkey = _sym("orderkey", _i64_t);

        auto red = _red(
            lineitem[orderkey], []() { return _f64(0); },
            [date](Expr s, Expr v) {
                auto l_extendedprice = _get(v, 4);
                auto l_discount = _get(v, 5);
                auto l_shipdate = _get(v, 9);

                auto new_s =
                    _add(s, _mul(l_extendedprice, _sub(_f64(1), l_discount)));
                return _sel(_gt(l_shipdate, _i64(date)), new_s, s);
            });
        auto red_sym = _sym("red", red);

        auto c_idx = _locate(customer, _get(orders[orderkey], 0));
        auto c_idx_sym = _sym("c_idx", c_idx);
        auto filter = _gte(c_idx_sym, _idx(0)) &
                      _lt(_get(orders[orderkey], 3), _i64(date)) &
                      _eq(_readdata(customer, c_idx_sym, 6), _i8(segment)) &
                      _gt(red_sym, _f64(0));
        auto filter_sym = _sym("filter", filter);
        auto pred =
            _in(orderkey, lineitem) & _in(orderkey, orders) & filter_sym;

        auto op = _op(vector<Sym>{orderkey}, pred, vector<Expr>{red_sym});
        auto op_sym = _sym("op", op);

        auto fn = _func("tpchquery3", op_sym,
                        vector<Sym>{lineitem, orders, customer});
        fn->tbl[c_idx_sym] = c_idx;
        fn->tbl[filter_sym] = filter;
        fn->tbl[op_sym] = op;
        fn->tbl[red_sym] = red;

        return fn;
    }

    ArrowTable* run()
    {
        ArrowTable* out;
        this->query_fn(&out, this->lineitem.get(), this->orders.get(),
                       this->customer.get());
        return out;
    }
};

struct TPCHQuery4 {
    using QueryFnTy = void (*)(int*, ArrowTable*, ArrowTable*);

    shared_ptr<ArrowTable2> lineitem;
    shared_ptr<ArrowTable2> orders;
    QueryFnTy query_fn;

    TPCHQuery4()
    {
        this->lineitem =
            load_arrow_file("../benchmark/arrow_data/lineitem.arrow", 2);
        this->orders =
            load_arrow_file("../benchmark/arrow_data/orders.arrow", 1);
        this->lineitem->build_index();
        this->orders->build_index();
        this->query_fn =
            compile_op<QueryFnTy>(this->build_op(700000000, 900000000));
    }

    shared_ptr<Func> build_op(int64_t start_date, int64_t end_date)
    {
        auto lineitem = _sym("lineitem", this->lineitem->get_data_type());
        auto orders = _sym("orders", this->orders->get_data_type());
        auto orderkey = _sym("orderkey", _i64_t);

        auto exists = _red(
            lineitem[orderkey], []() { return _false(); },
            [](Expr s, Expr v) {
                auto l_commitdate = _get(v, 10);
                auto l_receiptdate = _get(v, 11);

                return _or(_lt(l_commitdate, l_receiptdate), s);
            });
        auto exists_sym = _sym("exists", exists);

        auto filter = _gte(_get(orders[orderkey], 3), _i64(start_date)) &
                      _lt(_get(orders[orderkey], 3), _i64(end_date)) &
                      exists_sym;
        auto filter_sym = _sym("filter", filter);

        auto pred =
            _in(orderkey, orders) & _in(orderkey, lineitem) & filter_sym;
        auto red = _red(
            _op(vector<Sym>{orderkey}, pred,
                vector<Expr>{_get(orders[orderkey], 4)}),
            []() {
                return _new(
                    vector<Expr>{_i32(0), _i32(0), _i32(0), _i32(0), _i32(0)});
            },
            [](Expr s, Expr val) {
                auto v = _get(val, 1);
                return _new(vector<Expr>{
                    _sel(_eq(v, _i8(0)), _add(_get(s, 0), _i32(1)), _get(s, 0)),
                    _sel(_eq(v, _i8(1)), _add(_get(s, 1), _i32(1)), _get(s, 1)),
                    _sel(_eq(v, _i8(2)), _add(_get(s, 2), _i32(1)), _get(s, 2)),
                    _sel(_eq(v, _i8(3)), _add(_get(s, 3), _i32(1)), _get(s, 3)),
                    _sel(_eq(v, _i8(4)), _add(_get(s, 4), _i32(1)), _get(s, 4)),
                });
            });
        auto red_sym = _sym("red", red);

        auto fn = _func("tpchquery4", red_sym, vector<Sym>{lineitem, orders});
        fn->tbl[exists_sym] = exists;
        fn->tbl[filter_sym] = filter;
        fn->tbl[red_sym] = red;

        return fn;
    }

    vector<int> run()
    {
        vector<int> out(5);
        this->query_fn(out.data(), this->lineitem.get(), this->orders.get());
        return std::move(out);
    }
};

struct TPCHQuery6 {
    using QueryFnTy = void (*)(double*, ArrowTable*);

    shared_ptr<ArrowTable2> lineitem;
    QueryFnTy query_fn;

    TPCHQuery6()
    {
        this->lineitem =
            load_arrow_file("../benchmark/arrow_data/lineitem.arrow", 2);
        this->query_fn = compile_op<QueryFnTy>(
            this->build_op(820454400, 852076800, 0.05f, 24.5f), true);
    }

    shared_ptr<Func> build_op(int64_t start, int64_t end, double disc,
                              double quant)
    {
        auto vec_in_sym = _sym("lineitem", this->lineitem->get_data_type());
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

        auto fn = _func("tpchquery6", red_sym, vector<Sym>{vec_in_sym});
        fn->tbl[red_sym] = red;

        return fn;
    }

    double run()
    {
        double out;
        this->query_fn(&out, this->lineitem.get());
        return out;
    }
};

struct TPCDSQuery9 {
    using QueryFnTy = void (*)(void*, ArrowTable*);

    shared_ptr<ArrowTable2> store_sales;
    QueryFnTy query_fn;

    TPCDSQuery9()
    {
        this->store_sales =
            load_arrow_file("../benchmark/arrow_data/store_sales.arrow", 2);
        this->query_fn = compile_op<QueryFnTy>(this->build_op());
    }

    shared_ptr<Func> build_op()
    {
        auto vec_in_sym =
            _sym("store_sales", this->store_sales->get_data_type());
        auto red = _red(
            _subvec(vec_in_sym, _idx(0), _len(vec_in_sym, 1)),
            []() {
                return _new(vector<Expr>{
                    _new(vector<Expr>{_i32(0), _f64(0), _f64(0)}),
                    _new(vector<Expr>{_i32(0), _f64(0), _f64(0)}),
                    _new(vector<Expr>{_i32(0), _f64(0), _f64(0)}),
                    _new(vector<Expr>{_i32(0), _f64(0), _f64(0)}),
                    _new(vector<Expr>{_i32(0), _f64(0), _f64(0)}),
                });
            },
            [](Expr s, Expr v) {
                auto ss_quantity = _get(v, 9);
                auto ss_ext_tax = _get(v, 17);
                auto ss_net_paid_inc_tax = _get(v, 20);

                auto bucket1 = _get(s, 0);
                auto bucket2 = _get(s, 1);
                auto bucket3 = _get(s, 2);
                auto bucket4 = _get(s, 3);
                auto bucket5 = _get(s, 4);

                return _new(vector<Expr>{
                    _sel(_gte(ss_quantity, _i32(1)) &
                             _lte(ss_quantity, _i32(20)),
                         _new(vector<Expr>{
                             _add(_get(bucket1, 0), _i32(1)),
                             _add(_get(bucket1, 1), ss_ext_tax),
                             _add(_get(bucket1, 2), ss_net_paid_inc_tax),
                         }),
                         bucket1),
                    _sel(_gte(ss_quantity, _i32(21)) &
                             _lte(ss_quantity, _i32(40)),
                         _new(vector<Expr>{
                             _add(_get(bucket2, 0), _i32(1)),
                             _add(_get(bucket2, 1), ss_ext_tax),
                             _add(_get(bucket2, 2), ss_net_paid_inc_tax),
                         }),
                         bucket2),
                    _sel(_gte(ss_quantity, _i32(41)) &
                             _lte(ss_quantity, _i32(60)),
                         _new(vector<Expr>{
                             _add(_get(bucket3, 0), _i32(1)),
                             _add(_get(bucket3, 1), ss_ext_tax),
                             _add(_get(bucket3, 2), ss_net_paid_inc_tax),
                         }),
                         bucket3),
                    _sel(_gte(ss_quantity, _i32(61)) &
                             _lte(ss_quantity, _i32(80)),
                         _new(vector<Expr>{
                             _add(_get(bucket4, 0), _i32(1)),
                             _add(_get(bucket4, 1), ss_ext_tax),
                             _add(_get(bucket4, 2), ss_net_paid_inc_tax),
                         }),
                         bucket4),
                    _sel(_gte(ss_quantity, _i32(81)) &
                             _lte(ss_quantity, _i32(100)),
                         _new(vector<Expr>{
                             _add(_get(bucket5, 0), _i32(1)),
                             _add(_get(bucket5, 1), ss_ext_tax),
                             _add(_get(bucket5, 2), ss_net_paid_inc_tax),
                         }),
                         bucket5),
                });
            });
        auto red_sym = _sym("red", red);

        auto fn = _func("tpcdsquery9", red_sym, vector<Sym>{vec_in_sym});
        fn->tbl[red_sym] = red;
        return fn;
    }

    vector<int> run()
    {
        vector<int> out(100);
        this->query_fn(out.data(), this->store_sales.get());
        return out;
    }
};

struct TPCHQuery11 {
    using QueryFnTy = void (*)(ArrowTable**, ArrowTable*, ArrowTable*);

    shared_ptr<ArrowTable2> supplier;
    shared_ptr<ArrowTable2> partsupp;
    shared_ptr<ArrowTable2> supppart;
    QueryFnTy query_fn;

    TPCHQuery11()
    {
        this->supplier =
            load_arrow_file("../benchmark/arrow_data/supplier.arrow", 1);
        this->partsupp =
            load_arrow_file("../benchmark/arrow_data/partsupp.arrow", 2);
        this->supppart =
            load_arrow_file("../benchmark/arrow_data/supppart.arrow", 2);
        this->supplier->build_index();
        this->partsupp->build_index();
        this->query_fn = compile_op<QueryFnTy>(this->build_op(0, 0.0001));
    }

    shared_ptr<Func> build_op(int64_t nation_key, double fraction)
    {
        auto supplier = _sym("supplier", this->supplier->get_data_type());
        auto partsupp = _sym("partsupp", this->partsupp->get_data_type());
        auto partkey = _sym("partkey", _i64_t);
        auto suppkey = _sym("suppkey", _i64_t);

        auto value = _red(
            partsupp[partkey], []() { return _f64(0); },
            [nation_key, supplier](Expr s, Expr v) {
                auto skey = _get(v, 0);
                auto nkey = _get(supplier[skey], 2);
                auto cost = _get(v, 2);
                auto qty = _cast(_f64_t, _get(v, 1));
                return _sel(_eq(nkey, _i64(nation_key)),
                            _add(s, _mul(qty, cost)), s);
            });
        auto value_sym = _sym("value", value);
        auto filter = _gt(value_sym, _f64(0));
        auto filter_sym = _sym("filter", filter);
        auto op = _op(vector<Sym>{partkey}, _in(partkey, partsupp) & filter_sym,
                      vector<Expr>{value_sym});
        auto op_sym = _sym("op", op);

        auto threshold =
            _initval(vector<Sym>{op_sym},
                     _red(
                         op_sym, []() { return _f64(0); },
                         [](Expr s, Expr v) { return _add(s, _get(v, 1)); }));
        auto threshold_sym = _sym("threshold", threshold);

        auto partkey2 = _sym("partkey2", _i64_t);
        auto val = _get(op_sym[partkey2], 0);
        auto val_sym = _sym("val", val);
        auto filter2 = _gt(val_sym, threshold_sym);
        auto filter2_sym = _sym("filter2", filter2);
        auto op2 = _initval(
            vector<Sym>{threshold_sym},
            _op(vector<Sym>{partkey2}, _in(partkey2, op_sym) & filter2_sym,
                vector<Expr>{val_sym}));
        auto op2_sym = _sym("op2", op2);

        auto fn = _func("tpchquery11", op_sym, vector<Sym>{supplier, partsupp});
        fn->tbl[op_sym] = op;
        fn->tbl[value_sym] = value;
        fn->tbl[filter_sym] = filter;
        fn->tbl[threshold_sym] = threshold;
        fn->tbl[op2_sym] = op2;
        fn->tbl[filter2_sym] = filter2;
        fn->tbl[val_sym] = val;

        return fn;
    }

    ArrowTable* run()
    {
        ArrowTable* out;
        this->query_fn(&out, this->supplier.get(), this->partsupp.get());
        return out;
    }
};
