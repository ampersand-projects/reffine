#include "reffine/utils/utils.h"
#include "reffine/builder/reffiner.h"
#include "reffine/engine/memory.h"

using namespace reffine;
using namespace reffine::reffiner;

static void(*clone_fn)(ArrowTable**, ArrowTable*) = nullptr;

static shared_ptr<Func> clone_op(ArrowTable2* table)
{
    auto vtype = table->get_data_type();
    auto dim = vtype.dim;

    ASSERT(dim == 1);

    auto vec_in_sym = _sym("vec_in", vtype);
    auto names = table->get_col_names();
    auto iter = _sym(names[0], vtype.iterty());
    auto elem = vec_in_sym[iter];
    auto elem_sym = _sym("elem", elem);

    vector<Expr> out_exprs;
    vector<Sym> out_syms;
    vector<Expr> outs;
    for (size_t i = 1; i < vtype.dtypes.size(); i++) {
        auto col = elem_sym[i-1];
        auto col_sym = _sym(names[i], col);
        out_exprs.push_back(col);
        out_syms.push_back(col_sym);
        outs.push_back(col_sym);
    }

    auto op = _op(vector<Sym>{iter}, _in(iter, vec_in_sym), outs);
    auto op_sym = _sym("op", op);

    auto fn = _func("clone", op_sym, vector<Sym>{vec_in_sym});
    fn->tbl[elem_sym] = elem;
    for (size_t i = 1; i < vtype.dtypes.size(); i++) {
        fn->tbl[out_syms[i-1]] = out_exprs[i-1];
    }
    fn->tbl[op_sym] = op;

    return fn;
}

shared_ptr<ArrowTable2> clone_table(ArrowTable2* table)
{
    if (!clone_fn) {
        auto fn = clone_op(table);
        clone_fn = compile_op<void(*)(ArrowTable**, ArrowTable*)>(fn);
    }

    ArrowTable* out_tbl;
    clone_fn(&out_tbl, table);

    return memman.fetch_table(out_tbl);
}
