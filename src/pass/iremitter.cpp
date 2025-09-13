#include "reffine/pass/iremitter.h"

#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

static const auto FORALL = "\u2200";
static const auto REDCLE = "\u2295";
static const auto PHI = "\u0278";

CodeSeg IREmitter::visit(Sym sym)
{
    auto lhs = code(sym->name);

    if (this->ctx().in_sym_tbl.find(sym) != this->ctx().in_sym_tbl.end()) {
        auto rhs = eval(this->ctx().in_sym_tbl.at(sym));
        emit(nl(), lhs, " = ", rhs);
    }

    return lhs;
}

CodeSeg IREmitter::visit(StmtExprNode& e) { return eval(e.stmt); }

CodeSeg IREmitter::visit(Stmts& s)
{
    for (auto& stmt : s.stmts) { emit(nl(), eval(stmt)); }
    return code("");
}

CodeSeg IREmitter::visit(Call& e) { return code_func(e.name, e.args); }

CodeSeg IREmitter::visit(IfElse& s)
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

CodeSeg IREmitter::visit(Select& e)
{
    return code("(", eval(e.cond), " ? ", eval(e.true_body), " : ",
                eval(e.false_body), ")");
}

CodeSeg IREmitter::visit(Const& cnst)
{
    switch (cnst.type.btype) {
        case BaseType::BOOL:
            return code(cnst.val ? "true" : "false");
        case BaseType::INT8:
        case BaseType::INT16:
        case BaseType::INT32:
        case BaseType::INT64:
            return code(to_string((int64_t)cnst.val) + "i");
        case BaseType::UINT8:
        case BaseType::UINT16:
        case BaseType::UINT32:
        case BaseType::UINT64:
            return code(to_string((int64_t)cnst.val) + "i");
        case BaseType::FLOAT32:
        case BaseType::FLOAT64:
            return code(to_string(cnst.val) + "f");
        case BaseType::IDX:
            return code(to_string((int64_t)cnst.val) + "i");
        default:
            throw std::runtime_error("Invalid constant type");
    }
}

CodeSeg IREmitter::visit(Cast& e)
{
    return code("(", e.type.str(), ") ", eval(e.arg));
}

CodeSeg IREmitter::visit(Get& e)
{
    return code("(", eval(e.val), ")._", to_string(e.col));
}

CodeSeg IREmitter::visit(New& e)
{
    auto line = code("{");

    for (const auto& val : e.vals) { line->emit(eval(val), ", "); }
    line->emit("}");

    return line;
}

CodeSeg IREmitter::visit(NaryExpr& e)
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

CodeSeg IREmitter::visit(Op& op)
{
    auto line = code(FORALL, " ");
    line->emit(code_args("", op.iters, ""));

    line->emit(": [");
    auto parent1 = enter_block();
    emit(nl(), "return ", eval(op.pred));
    auto child1 = exit_block(parent1);
    line->emit(child1, nl(), "] {");

    auto parent2 = enter_block();
    emit(nl(), "return ", code_args("{", op.outputs, "}"));
    auto child2 = exit_block(parent2);
    line->emit(child2);
    line->emit(nl(), "}");

    return line;
}

CodeSeg IREmitter::visit(Element& elem)
{
    return code(eval(elem.vec), code_args("[", elem.iters, "]"));
}

CodeSeg IREmitter::visit(NotNull& e) { return code(eval(e.elem), "!=", PHI); }

CodeSeg IREmitter::visit(Reduce& red)
{
    auto state_val = red.init();
    auto val = _sym("val", red.op.type.rowty());
    auto state = _sym("state", state_val->type);
    auto state2 = red.acc(state, val);

    auto line = code(REDCLE, " {");

    auto parent = enter_block();
    emit(nl(), eval(this->tmp_expr(red.op)), ", ", nl());
    emit("state <- ", eval(state_val), ", ", nl());
    emit("state <- ", eval(state2));

    auto child = exit_block(parent);
    line->emit(child, ", ", nl(), "}");

    return line;
}

CodeSeg IREmitter::visit(Alloc& e)
{
    return code("alloc ", e.type.deref().str());
}

CodeSeg IREmitter::visit(Load& e) { return code_func("*", {e.addr}); }

CodeSeg IREmitter::visit(Store& s)
{
    return code("*(", eval(s.addr), ") = ", eval(s.val));
}

CodeSeg IREmitter::visit(AtomicOp& e)
{
    switch (e.op) {
        case MathOp::ADD:
            return code_func("atomic_add", {e.addr, e.val});
        case MathOp::SUB:
            return code_func("atomic_sub", {e.addr, e.val});
        case MathOp::MAX:
            return code_func("atomic_max", {e.addr, e.val});
        case MathOp::MIN:
            return code_func("atomic_min", {e.addr, e.val});
        case MathOp::AND:
            return code_func("atomic_and", {e.addr, e.val});
        case MathOp::OR:
            return code_func("atomic_or", {e.addr, e.val});
        default:
            throw std::runtime_error("Invalid atomic operation");
    }
}

CodeSeg IREmitter::visit(StructGEP& e)
{
    return code_func("structgep", {e.addr, _idx(e.col)});
}

CodeSeg IREmitter::visit(ThreadIdx&) { return code("tidx"); }

CodeSeg IREmitter::visit(BlockIdx&) { return code("bidx"); }

CodeSeg IREmitter::visit(BlockDim&) { return code("bdim"); }

CodeSeg IREmitter::visit(GridDim&) { return code("gdim"); }

CodeSeg IREmitter::visit(Loop& e)
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

CodeSeg IREmitter::visit(FetchDataPtr& e)
{
    return code(eval(e.vec), "[", eval(e.idx), "]._" + to_string(e.col));
}

CodeSeg IREmitter::visit(NoOp&) { return code(""); }

CodeSeg IREmitter::visit(Func& fn)
{
    auto new_ctx = make_unique<IREmitterCtx>(fn.tbl);
    this->switch_ctx(new_ctx);

    emit("def ", fn.name, code_args("(", fn.inputs, ")"), " {");

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

string IREmitter::Build(Stmt stmt)
{
    IREmitter emitter(make_unique<IREmitterCtx>());
    return emitter.eval(stmt)->to_string(-1);
}
