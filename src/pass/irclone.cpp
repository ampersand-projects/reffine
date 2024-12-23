#include "reffine/pass/irclone.h"

using namespace std;
using namespace reffine;

shared_ptr<Op> IRClone::visit_op(Op& op)
{
    vector<Sym> new_idxs;
    for (auto& old_idx : op.idxs) {
        auto new_idx = make_shared<SymNode>(old_idx->name, old_idx);
        new_idxs.push_back(new_idx);
        map_sym(old_idx, new_idx);
    }

    vector<Expr> new_preds;
    for (auto& old_pred : op.preds) { new_preds.push_back(eval(old_pred)); }

    vector<Expr> new_outputs;
    for (auto& old_output : op.outputs) {
        new_outputs.push_back(eval(old_output));
    }

    return make_shared<Op>(new_idxs, new_preds, new_outputs);
}

Expr IRClone::visit(Reduce& red)
{
    auto new_op = IRClone::visit_op(red.op);
    return make_shared<Reduce>(*new_op, red.init, red.acc);
}

Expr IRClone::visit(Op& op) { return IRClone::visit_op(op); }

Expr IRClone::visit(Element& elem)
{
    vector<Expr> new_idxs;
    for (auto& old_idx : elem.idxs) { new_idxs.push_back(eval(old_idx)); }

    return make_shared<Element>(eval(elem.vec), new_idxs);
}

Expr IRClone::visit(NotNull& expr)
{
    return make_shared<NotNull>(eval(expr.elem));
}

Expr IRClone::visit(NaryExpr& nexpr)
{
    vector<Expr> new_args;
    for (auto& old_arg : nexpr.args) { new_args.push_back(eval(old_arg)); }

    return make_shared<NaryExpr>(nexpr.type, nexpr.op, new_args);
}

Expr IRClone::visit(Get& get)
{
    return make_shared<Get>(eval(get.val), get.col);
}

Expr IRClone::visit(New& _new)
{
    vector<Expr> new_vals;
    for (auto& val : _new.vals) { new_vals.push_back(eval(val)); }

    return make_shared<New>(new_vals);
}

Expr IRClone::visit(Cast& cast)
{
    return make_shared<Cast>(cast.type, eval(cast.arg));
}

Expr IRClone::visit(Const& cnst)
{
    return make_shared<Const>(cnst.type.btype, cnst.val);
}

Expr IRClone::visit(Select& select)
{
    return make_shared<Select>(eval(select.cond), eval(select.true_body),
                               eval(select.false_body));
}

Expr IRClone::visit(Call& call)
{
    vector<Expr> new_args;
    for (auto& old_arg : call.args) { new_args.push_back(eval(old_arg)); }

    return make_shared<Call>(call.name, call.type, new_args);
}

void IRClone::visit(Func& func)
{
    // Populate loop function inputs
    for (auto& old_input : func.inputs) {
        auto new_input = make_shared<SymNode>(old_input->name, old_input);
        _ctx.new_func->inputs.push_back(new_input);
        map_sym(old_input, new_input);
    }

    _ctx.new_func->output = eval(func.output);
}

tuple<Sym, Expr> IRClone::visit(Sym old_sym, Expr old_val)
{
    auto new_val = eval(old_val);
    auto new_sym = make_shared<SymNode>(old_sym->name, old_sym);

    return {new_sym, new_val};
}
