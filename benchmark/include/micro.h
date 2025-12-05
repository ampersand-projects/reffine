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

struct MicroBench {
    using QueryFnTy = void (*)(ArrowTable**, ArrowTable*, ArrowTable*);
    using SumQueryFnTy = void (*)(int64_t*, ArrowTable*, ArrowTable*);

    shared_ptr<ArrowTable2> left;
    shared_ptr<ArrowTable2> right;
    shared_ptr<ArrowTable2> in;
    QueryFnTy select_fn;
    QueryFnTy ijoin_fn;
    QueryFnTy ojoin_fn;
    SumQueryFnTy sum_fn;

    MicroBench()
    {
        this->left =
            load_arrow_file("../benchmark/arrow_data/fake_data.arrow", 1);
        this->right =
            load_arrow_file("../benchmark/arrow_data/fake_data.arrow", 1);
        this->in =
            load_arrow_file("../benchmark/arrow_data/fake_data.arrow", 2);
        this->left->build_index();
        this->right->build_index();
        this->select_fn = compile_op<QueryFnTy>(this->select_op());
        this->ijoin_fn = compile_op<QueryFnTy>(this->ijoin_op());
        this->ojoin_fn = compile_op<QueryFnTy>(this->ojoin_op());
        this->sum_fn = compile_op<SumQueryFnTy>(this->sum_op());
    }

    shared_ptr<Func> select_op()
    {
        auto vec_in = _sym("vec_in", this->left->get_data_type());
        auto t_sym = _sym("t", _i64_t);

        auto val = _get(vec_in[t_sym], 0);
        auto cond = _eq(_mod(t_sym, _i64(2)), _i64(0));
        auto cond_sym = _sym("cond", cond);
        auto op = _op(vector<Sym>{t_sym},
                      _in(t_sym, vec_in) & cond_sym,
                      vector<Expr>{});
        auto op_sym = _sym("op", op);

        auto fn = _func("selectbench", op_sym, vector<Sym>{vec_in});
        fn->tbl[op_sym] = op;
        fn->tbl[cond_sym] = cond;

        return fn;
    }

    shared_ptr<Func> ijoin_op()
    {
        auto left = _sym("left", this->left->get_data_type());
        auto right = _sym("right", this->right->get_data_type());
        auto t_sym = _sym("t", _i64_t);

        auto lval = _get(left[t_sym], 0);
        auto rval = _get(right[t_sym], 0);
        auto diff = _sub(lval, rval);
        auto diff_sym = _sym("diff", diff);
        auto op = _op(vector<Sym>{t_sym},
                      _in(t_sym, left) & _in(t_sym, right),
                      vector<Expr>{diff_sym});
        auto op_sym = _sym("op", op);

        auto fn = _func("ijoinbench", op_sym, vector<Sym>{left, right});
        fn->tbl[op_sym] = op;
        fn->tbl[diff_sym] = diff;

        return fn;
    }

    shared_ptr<Func> ojoin_op()
    {
        auto left = _sym("left", this->left->get_data_type());
        auto right = _sym("right", this->right->get_data_type());
        auto t_sym = _sym("t", _i64_t);

        auto lval = _get(left[t_sym], 0);
        auto rval = _get(right[t_sym], 0);
        auto diff = _sub(lval, rval);
        auto diff_sym = _sym("diff", diff);
        auto op = _op(vector<Sym>{t_sym},
                      _in(t_sym, left) | _in(t_sym, right),
                      vector<Expr>{diff_sym});
        auto op_sym = _sym("op", op);

        auto fn = _func("ojoinbench", op_sym, vector<Sym>{left, right});
        fn->tbl[op_sym] = op;
        fn->tbl[diff_sym] = diff;

        return fn;
    }

    shared_ptr<Func> sum_op()
    {
        auto vec_in = _sym("vec_in", this->in->get_data_type());
        auto red = _red(_subvec(vec_in, _idx(0), _len(vec_in, 1)),
            []() { return _i64(0); },
            [](Expr s, Expr v) { return _add(_get(v, 0), s); }
        );
        auto red_sym = _sym("red", red);
        auto fn = _func("sumbench", red_sym, vector<Sym>{vec_in});
        fn->tbl[red_sym] = red;

        return fn;
    }

    ArrowTable* run(string q)
    {
        ArrowTable* out = nullptr;
        if (q == "select") {
            this->select_fn(&out, this->left.get(), this->right.get());
        } else if (q == "inner") {
            this->ijoin_fn(&out, this->left.get(), this->right.get());
        } else if (q == "outer") {
            this->ojoin_fn(&out, this->left.get(), this->right.get());
        } else if (q == "sum") {
            long res;
            this->sum_fn(&res, this->left.get(), this->right.get());
            cout << "Res: " << res << endl;
        }
        return out;
    }
};
