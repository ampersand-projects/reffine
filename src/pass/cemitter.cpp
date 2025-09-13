#include "reffine/pass/cemitter.h"
#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

CodeSeg CEmitter::visit(Sym sym)
{
    auto lhs = code(sym->name);

    if (this->ctx().in_sym_tbl.find(sym) != this->ctx().in_sym_tbl.end()) {
        auto rhs = eval(this->ctx().in_sym_tbl.at(sym));
        emit(nl(), lhs, " = ", rhs);
    }

    return lhs;
}

CodeSeg CEmitter::visit(Stmts& s)
{
    for (auto& stmt : s.stmts) { emit(nl(), eval(stmt), ";"); }
    return code("");
}

CodeSeg CEmitter::visit(Call& e) { return code_func(e.name, e.args); }

CodeSeg CEmitter::visit(IfElse& s)
{
    emit(nl(), "if (", eval(s.cond), ") {");

    auto parent1 = enter_block();
    emit(eval(s.true_body));
    auto child1 = exit_block(parent1);
    emit(child1);

    emit(nl(), "} else {");

    auto parent2 = enter_block();
    emit(eval(s.false_body));
    auto child2 = exit_block(parent2);
    emit(child2);

    emit(nl(), "}");

    return code("");
}

CodeSeg CEmitter::visit(Select& e)
{
    return code("(", eval(e.cond), " ? ", eval(e.true_body), " : ",
                eval(e.false_body), ")");
}

CodeSeg CEmitter::visit(Get&)
{
    return code("get()");
}

CodeSeg CEmitter::visit(Const& cnst)
{
    switch (cnst.type.btype) {
        case BaseType::BOOL:
            return code(cnst.val ? "true" : "false");
        case BaseType::INT8:
        case BaseType::INT16:
        case BaseType::INT32:
        case BaseType::INT64:
            return code(to_string((int64_t)cnst.val));
        case BaseType::UINT8:
        case BaseType::UINT16:
        case BaseType::UINT32:
        case BaseType::UINT64:
            return code(to_string((uint64_t)cnst.val));
        case BaseType::FLOAT32:
            return code(to_string(cnst.val) + "f");
        case BaseType::FLOAT64:
            return code(to_string(cnst.val));
        case BaseType::IDX:
            return code(to_string((int64_t)cnst.val));
        default:
            throw std::runtime_error("Invalid constant type");
    }
}

CodeSeg CEmitter::visit(Cast& e)
{
    return code("(", e.type.str(), ") ", eval(e.arg));
}

CodeSeg CEmitter::visit(NaryExpr& e)
{
    switch (e.op) {
        case MathOp::ADD:
            return code_binary(e.arg(0), "+", e.arg(1));
        case MathOp::SUB:
            return code_binary(e.arg(0), "-", e.arg(1));
        case MathOp::MUL:
            return code_binary(e.arg(0), "*", e.arg(1));
        case MathOp::DIV:
            return code_binary(e.arg(0), "/", e.arg(1));
        case MathOp::MAX:
            return code_func("max", {e.arg(0), e.arg(1)});
        case MathOp::MIN:
            return code_func("min", {e.arg(0), e.arg(1)});
        case MathOp::MOD:
            return code_binary(e.arg(0), "%", e.arg(1));
        case MathOp::ABS:
            return code("|", eval(e.arg(0)), "|");
        case MathOp::NEG:
            return code("-", eval(e.arg(0)));
        case MathOp::SQRT:
            return code_func("sqrt", {e.arg(0)});
        case MathOp::POW:
            return code_binary(e.arg(0), "^", e.arg(1));
        case MathOp::CEIL:
            return code_func("ceil", {e.arg(0)});
        case MathOp::FLOOR:
            return code_func("floor", {e.arg(0)});
        case MathOp::EQ:
            return code_binary(e.arg(0), "==", e.arg(1));
        case MathOp::NOT:
            return code("!", eval(e.arg(0)));
        case MathOp::AND:
            return code_binary(e.arg(0), "&&", e.arg(1));
        case MathOp::OR:
            return code_binary(e.arg(0), "||", e.arg(1));
        case MathOp::LT:
            return code_binary(e.arg(0), "<", e.arg(1));
        case MathOp::LTE:
            return code_binary(e.arg(0), "<=", e.arg(1));
        case MathOp::GT:
            return code_binary(e.arg(0), ">", e.arg(1));
        case MathOp::GTE:
            return code_binary(e.arg(0), ">=", e.arg(1));
        default:
            throw std::runtime_error("Invalid math operation");
    }
}

CodeSeg CEmitter::visit(Alloc& e)
{
    return code("(", e.type.deref().str(), ")", " 1");
}

CodeSeg CEmitter::visit(Load& e) { return code_func("*", {e.addr}); }

CodeSeg CEmitter::visit(Store& s)
{
    return code("*(", eval(s.addr), ") = ", eval(s.val));
}

CodeSeg CEmitter::visit(Loop& e)
{
    if (e.init) { emit(nl(), eval(e.init)); }

    emit(nl(), "while(1) {");
    auto parent = enter_block();

    emit(nl(), "if (", eval(e.exit_cond), ") break", nl());
    if (e.body_cond) {
        emit(nl(), "if (!", eval(e.body_cond), ") continue", nl());
    }
    emit(nl(), eval(e.body));
    if (e.incr) { emit(eval(e.incr)); }

    auto child = exit_block(parent);
    emit(child, nl(), "}");

    if (e.post) { emit(nl(), eval(e.post)); }
    emit(nl());

    return eval(e.output);
}

CodeSeg CEmitter::visit(StructGEP&)
{
    return code("gep");
}

CodeSeg CEmitter::visit(FetchDataPtr& e)
{
    return code(eval(e.vec), "[", eval(e.idx), "]._" + to_string(e.col));
}

CodeSeg CEmitter::visit(NoOp&) { return code(""); }

CodeSeg CEmitter::visit(Func& fn)
{
    ASSERT(fn.output->type.is_void());

    auto new_ctx = make_unique<IREmitterCtx>(fn.tbl);
    this->switch_ctx(new_ctx);

    emit("void ", fn.name, code_args("(", fn.inputs, ")"), " {");

    auto parent = enter_block();
    if (fn.output->type.is_void()) {
        emit(nl(), eval(fn.output));
    } else {
        emit(nl(), "return ", eval(fn.output));
    }
    auto child = exit_block(parent);

    emit(child);
    emit(nl(), "}");

    return this->_code;
}

string CEmitter::Build(Stmt stmt)
{
    CEmitter cemitter(make_unique<IREmitterCtx>());
    return cemitter.eval(stmt)->to_string(-1);
}
