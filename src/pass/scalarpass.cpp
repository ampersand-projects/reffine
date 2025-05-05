#include "reffine/pass/scalarpass.h"

#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

Expr LoadStoreExpand::visit(Load& load)
{
    if (load.type.is_struct()) {
        auto addr = eval(load.addr);

        vector<Expr> vals;
        for (size_t i=0; i<load.type.dtypes.size(); i++) {
            vals.push_back(_load(_structgep(addr, i)));
        }

        return _new(vals);
    } else {
        return IRClone::visit(load);
    }
}

Expr LoadStoreExpand::visit(Store& store)
{
    if (store.val->type.is_struct()) {
        auto addr = eval(store.addr);
        auto val = eval(store.val);

        vector<Stmt> stmt_list;
        for (size_t i=0; i<store.val->type.dtypes.size(); i++) {
            auto val_addr = _structgep(addr, i);
            stmt_list.push_back(_store(_structgep(addr, i), _get(val, i)));
        }

        return _stmtexpr(_stmts(stmt_list));
    } else {
        return IRClone::visit(store);
    }
}

shared_ptr<Func> LoadStoreExpand::Build(shared_ptr<Func> func)
{
    auto new_func = _func(func->name, nullptr, vector<Sym>{});

    LoadStoreExpandCtx ctx(func, new_func);
    LoadStoreExpand pass(ctx);
    func->Accept(pass);

    return new_func;
}
