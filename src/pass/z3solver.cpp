#include "reffine/pass/z3solver.h"

using namespace reffine;

void Z3Solver::Visit(SymNode& s)
{
    switch (s.type.btype) {
        case BaseType::BOOL:
            assign(ctx().bool_const(s.name.c_str()));
            break;
        case BaseType::INT8:
        case BaseType::INT16:
        case BaseType::INT32:
        case BaseType::INT64:
        case BaseType::UINT8:
        case BaseType::UINT16:
        case BaseType::UINT32:
        case BaseType::UINT64:
        case BaseType::IDX:
            assign(ctx().int_const(s.name.c_str()));
            break;
        case BaseType::FLOAT32:
        case BaseType::FLOAT64:
            assign(ctx().real_const(s.name.c_str()));
            break;
        default:
            throw std::runtime_error("Invalid constant type");
    }
}

void Z3Solver::Visit(Const& cnst)
{
    switch (cnst.type.btype) {
        case BaseType::BOOL:
            assign(ctx().bool_val(cnst.val));
            break;
        case BaseType::INT8:
        case BaseType::INT16:
        case BaseType::INT32:
        case BaseType::INT64:
        case BaseType::UINT8:
        case BaseType::UINT16:
        case BaseType::UINT32:
        case BaseType::UINT64:
        case BaseType::IDX:
            assign(ctx().int_val((int64_t)cnst.val));
            break;
        case BaseType::FLOAT32:
        case BaseType::FLOAT64:
            assign(ctx().fpa_val(cnst.val));
            break;
        default:
            throw std::runtime_error("Invalid constant type");
    }
}

void Z3Solver::Visit(NaryExpr& e)
{
    switch (e.op) {
        case MathOp::ADD:
            assign(eval(e.arg(0)) + eval(e.arg(1)));
            break;
        case MathOp::SUB:
            assign(eval(e.arg(0)) - eval(e.arg(1)));
            break;
        case MathOp::MUL:
            assign(eval(e.arg(0)) * eval(e.arg(1)));
            break;
        case MathOp::DIV:
            assign(eval(e.arg(0)) / eval(e.arg(1)));
            break;
        case MathOp::MAX:
            assign(z3::max(eval(e.arg(0)), eval(e.arg(1))));
            break;
        case MathOp::MIN:
            assign(z3::min(eval(e.arg(0)), eval(e.arg(1))));
            break;
        case MathOp::MOD:
            assign(z3::mod(eval(e.arg(0)), eval(e.arg(1))));
            break;
        case MathOp::ABS:
            assign(z3::abs(eval(e.arg(0))));
            break;
        case MathOp::NEG:
            assign(-eval(e.arg(0)));
            break;
        case MathOp::EQ:
            assign(eval(e.arg(0)) == eval(e.arg(1)));
            break;
        case MathOp::LT:
            assign(eval(e.arg(0)) < eval(e.arg(1)));
            break;
        case MathOp::LTE:
            assign(eval(e.arg(0)) <= eval(e.arg(1)));
            break;
        case MathOp::GT:
            assign(eval(e.arg(0)) > eval(e.arg(1)));
            break;
        case MathOp::GTE:
            assign(eval(e.arg(0)) >= eval(e.arg(1)));
            break;
        case MathOp::NOT:
            assign(!eval(e.arg(0)));
            break;
        case MathOp::AND:
            assign(eval(e.arg(0)) && eval(e.arg(1)));
            break;
        case MathOp::OR:
            assign(eval(e.arg(0)) || eval(e.arg(1)));
            break;
        case MathOp::IMPLIES:
            assign(z3::implies(eval(e.arg(0)), eval(e.arg(1))));
            break;
        case MathOp::FORALL:
            assign(z3::forall(eval(e.arg(0)), eval(e.arg(1))));
            break;
        case MathOp::EXISTS:
            assign(z3::exists(eval(e.arg(0)), eval(e.arg(1))));
            break;
        default:
            throw std::runtime_error("Invalid math operation");
    }
}

z3::expr Z3Solver::eval(Expr expr)
{
    z3::expr new_val = ctx().bool_val(true);

    swap(new_val, val());
    expr->Accept(*this);
    swap(val(), new_val);

    return new_val;
}

z3::check_result Z3Solver::check(Expr conj)
{
    s().add(eval(conj));
    return s().check();
}

z3::expr Z3Solver::get(Expr val) { return s().get_model().eval(eval(val)); }

z3::expr Z3Solver::solve(Expr conj, Expr val)
{
    check(conj);
    return get(val);
}
