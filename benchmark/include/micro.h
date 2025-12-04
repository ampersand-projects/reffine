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

struct SelectBench {
    using QueryFnTy = void (*)(ArrowTable**, ArrowTable*);

    shared_ptr<ArrowTable2> in;
    QueryFnTy query_fn;

    SelectBench()
    {
        this->in =
            load_arrow_file("../benchmark/arrow_data/fake_data.arrow", 1);
        this->in->build_index();
        this->query_fn = compile_op<QueryFnTy>(this->build_op());
    }

    shared_ptr<Func> build_op()
    {
        auto vec_in = _sym("vec_in", this->in->get_data_type());
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

    ArrowTable* run()
    {
        ArrowTable* out;
        this->query_fn(&out, this->in.get());
        return out;
    }
};
