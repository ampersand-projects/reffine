#include "reffine/pass/scalarpass.h"

#include "reffine/builder/reffiner.h"
#include "reffine/pass/printer.h"

using namespace reffine;
using namespace reffine::reffiner;

Expr LoadStoreExpand::visit(Load& load)
{
    if (load.type.is_struct()) {
        auto addr = eval(load.addr);

        vector<Expr> vals;
        for (size_t i = 0; i < load.type.dtypes.size(); i++) {
            vals.push_back(eval(_load(_structgep(addr, i))));
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

        vector<Expr> stmt_list;
        for (size_t i = 0; i < store.val->type.dtypes.size(); i++) {
            stmt_list.push_back(
                eval(_store(_structgep(addr, i), _get(val, i))));
        }

        return _stmts(stmt_list);
    } else {
        return IRClone::visit(store);
    }
}

Expr NewGetElimination::visit(Select& sel)
{
    auto new_cond = eval(sel.cond);
    auto new_true_body = eval(sel.true_body);
    auto new_false_body = eval(sel.false_body);
    auto new_sel = _sel(new_cond, new_true_body, new_false_body);

    if (new_sel->type.is_struct()) {
        vector<Expr> new_sel_exprs;

        for (size_t i = 0; i < new_sel->type.dtypes.size(); i++) {
            auto new_true_body_i = _new_get_map.at(new_true_body)[i];
            auto new_false_body_i = _new_get_map.at(new_false_body)[i];
            new_sel_exprs.push_back(
                eval(_sel(new_cond, new_true_body_i, new_false_body_i)));
        }

        _new_get_map[new_sel] = new_sel_exprs;
    }

    return new_sel;
}

Expr NewGetElimination::visit(New& expr)
{
    vector<Expr> new_vals;
    for (auto& val : expr.vals) { new_vals.push_back(eval(val)); }

    auto new_expr = _new(new_vals);
    _new_get_map[new_expr] = new_vals;

    return new_expr;
}

Expr NewGetElimination::visit(Get& get)
{
    auto val = eval(get.val);

    if (_new_get_map.find(val) != _new_get_map.end()) {
        return _new_get_map[val][get.col];
    } else {  // might be a symbol to New expression
        auto maybe_sym = static_pointer_cast<SymNode>(val);

        if (this->ctx().out_sym_tbl.find(maybe_sym) !=
            this->ctx().out_sym_tbl.end()) {
            auto expr = this->ctx().out_sym_tbl.at(maybe_sym);
            return _new_get_map[expr][get.col];
        } else {
            throw runtime_error("Unable to eliminate Get expression");
        }
    }
}

Expr NewGetElimination::visit(Func& func)
{
    // Struct type inputs are not supported yet
    for (const auto& input : func.inputs) { ASSERT(!input->type.is_struct()); }

    return IRClone::visit(func);
}
