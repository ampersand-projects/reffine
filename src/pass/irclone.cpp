#include "reffine/builder/reffiner.h"
#include "reffine/pass/irclone.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

shared_ptr<Op> IRClone::visit_op(Op& op)
{
    vector<Sym> new_iters;
    for (auto& old_iter : op.iters) {
        auto new_iter = _sym(old_iter->name, old_iter);
        new_iters.push_back(new_iter);
        this->map_sym(old_iter, new_iter);
    }

    auto new_pred = eval(op.pred);

    vector<Expr> new_outputs;
    for (auto& old_output : op.outputs) {
        new_outputs.push_back(eval(old_output));
    }

    return _op(new_iters, new_pred, new_outputs);
}

Expr IRClone::visit(Reduce& red)
{
    auto new_op = IRClone::visit_op(red.op);
    return _red(*new_op, red.init, red.acc);
}

Expr IRClone::visit(Op& op) { return IRClone::visit_op(op); }

Expr IRClone::visit(Element& elem)
{
    vector<Expr> new_iters;
    for (auto& old_iter : elem.iters) { new_iters.push_back(eval(old_iter)); }

    return _elem(eval(elem.vec), new_iters);
}

Expr IRClone::visit(NotNull& not_null)
{
    return _notnull(eval(not_null.elem));
}

Expr IRClone::visit(NaryExpr& nexpr)
{
    vector<Expr> new_args;
    for (auto& old_arg : nexpr.args) { new_args.push_back(eval(old_arg)); }

    return _nary(nexpr.type, nexpr.op, new_args);
}

Expr IRClone::visit(Get& get)
{
    return _get(eval(get.val), get.col);
}

Expr IRClone::visit(New& new_expr)
{
    vector<Expr> new_vals;
    for (auto& val : new_expr.vals) { new_vals.push_back(eval(val)); }

    return _new(new_vals);
}

Expr IRClone::visit(Cast& cast)
{
    return _cast(cast.type, eval(cast.arg));
}

Expr IRClone::visit(Const& cnst)
{
    return _const(cnst.type, cnst.val);
}

Expr IRClone::visit(Select& select)
{
    return _sel(eval(select.cond), eval(select.true_body), eval(select.false_body));
}

Expr IRClone::visit(Call& call)
{
    vector<Expr> new_args;
    for (auto& old_arg : call.args) { new_args.push_back(eval(old_arg)); }

    return _call(call.name, call.type, new_args);
}

void IRClone::visit(Func& func)
{
    // Populate loop function inputs
    for (auto& old_input : func.inputs) {
        auto new_input = _sym(old_input->name, old_input);
        _irclonectx.new_func->inputs.push_back(new_input);
        this->map_sym(old_input, new_input);
    }

    _irclonectx.new_func->output = eval(func.output);
}

Expr IRClone::visit(Sym old_sym)
{
    return _sym(old_sym->name, old_sym);
}

shared_ptr<Func> IRClone::Build(shared_ptr<Func> func)
{
    auto new_func = _func(func->name, nullptr, vector<Sym>{});

    IRCloneCtx ctx(func, new_func);
    IRClone irclone(ctx);
    func->Accept(irclone);

    return new_func;
}
