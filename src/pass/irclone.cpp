#include "reffine/pass/irclone.h"

using namespace std;
using namespace reffine;

shared_ptr<Op> IRClone::visit_op(Op& op)
{
    vector<Sym> new_iters;
    for (auto& old_iter : op.iters) {
        auto new_iter = make_shared<SymNode>(old_iter->name, old_iter);
        new_iters.push_back(new_iter);
        map_sym(old_iter, new_iter);
    }

    auto new_pred = eval(op.pred);

    vector<Expr> new_outputs;
    for (auto& old_output : op.outputs) {
        new_outputs.push_back(eval(old_output));
    }

    return make_shared<Op>(new_iters, new_pred, new_outputs);
}

Expr IRClone::visit(Reduce& red)
{
    auto new_op = IRClone::visit_op(red.op);
    return make_shared<Reduce>(*new_op, red.init, red.acc);
}

Expr IRClone::visit(Op& op) { return IRClone::visit_op(op); }

Expr IRClone::visit(Element& elem)
{
    vector<Expr> new_iters;
    for (auto& old_iter : elem.iters) { new_iters.push_back(eval(old_iter)); }

    return make_shared<Element>(eval(elem.vec), new_iters);
}

Expr IRClone::visit(NotNull& not_null)
{
    return make_shared<NotNull>(eval(not_null.elem));
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
        _irclonectx.new_func->inputs.push_back(new_input);
        this->map_sym(old_input, new_input);
    }

    _irclonectx.new_func->output = eval(func.output);
}

Expr IRClone::visit(Sym old_sym)
{
    return make_shared<SymNode>(old_sym->name, old_sym);
}

shared_ptr<Func> IRClone::Build(shared_ptr<Func> func)
{
    auto new_func = make_shared<Func>(func->name, nullptr, vector<Sym>{});

    IRCloneCtx ctx(func, new_func);
    IRClone irclone(ctx);
    func->Accept(irclone);

    return new_func;
}
