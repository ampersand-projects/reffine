#include "reffine/pass/printer2.h"

#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

static const auto FORALL = "\u2200";
static const auto REDCLE = "\u2295";
static const auto PHI = "\u0278";
// static const auto IN = "\u2208";

CodeSeg IRPrinter2::visit(Sym sym)
{
    auto lhs = code(sym->name);

    if (this->ctx().in_sym_tbl.find(sym) != this->ctx().in_sym_tbl.end()) {
        auto rhs = eval(this->ctx().in_sym_tbl.at(sym));
        emit(nl(), lhs, " = ", rhs);
    }

    return lhs;
}

CodeSeg IRPrinter2::visit(Stmts& s)
{
    for (auto& stmt : s.stmts) { emit(nl(), eval(stmt)); }
    return code("");
}

CodeSeg IRPrinter2::visit(Call& e) { return code_func(e.name, e.args); }

CodeSeg IRPrinter2::visit(IfElse& s)
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

CodeSeg IRPrinter2::visit(Select& e)
{
    return code("(", eval(e.cond), " ? ", eval(e.true_body), " : ",
                eval(e.false_body), ")");
}

CodeSeg IRPrinter2::visit(Const& cnst)
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

CodeSeg IRPrinter2::visit(Cast& e)
{
    return code("(", e.type.str(), ") ", eval(e.arg));
}

CodeSeg IRPrinter2::visit(Get& e)
{
    return code("(", eval(e.val), ")._", to_string(e.col));
}

CodeSeg IRPrinter2::visit(New& e)
{
    auto line = code("{");

    for (const auto& val : e.vals) { line->emit(eval(val), ", "); }
    line->emit("}");

    return line;
}

CodeSeg IRPrinter2::visit(NaryExpr& e)
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
        case MathOp::IMPLIES:
            return code_binary(e.arg(0), "=>", e.arg(1));
        case MathOp::IFF:
            return code_binary(e.arg(0), "<=>", e.arg(1));
        case MathOp::FORALL:
            return code_func("forall", e.args);
        case MathOp::EXISTS:
            return code_func("exists", e.args);
        default:
            throw std::runtime_error("Invalid math operation");
    }
}

CodeSeg IRPrinter2::visit(Op& op)
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

CodeSeg IRPrinter2::visit(Element& elem)
{
    return code(eval(elem.vec), "[", eval(elem.iter), "]");
}

CodeSeg IRPrinter2::visit(Lookup& e)
{
    return code_func("lookup", {e.vec, e.idx});
}

CodeSeg IRPrinter2::visit(In& e)
{
    return code(eval(e.vec), "[", eval(e.iter), "] !=", PHI);
}

CodeSeg IRPrinter2::visit(Reduce& red)
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

CodeSeg IRPrinter2::visit(Alloc& e)
{
    return code("alloc ", e.type.deref().str(), " ", eval(e.size));
}

CodeSeg IRPrinter2::visit(Load& e)
{
    return code_func("*", {e.addr, e.offset});
}

CodeSeg IRPrinter2::visit(Store& s)
{
    emit(nl(), "*(", eval(s.addr), " + ", eval(s.offset), ") = ", eval(s.val));
    return code("");
}

CodeSeg IRPrinter2::visit(AtomicOp& e)
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

CodeSeg IRPrinter2::visit(StructGEP& e)
{
    return code_func("structgep", {e.addr, _idx(e.col)});
}

CodeSeg IRPrinter2::visit(ThreadIdx&) { return code("tidx"); }

CodeSeg IRPrinter2::visit(BlockIdx&) { return code("bidx"); }

CodeSeg IRPrinter2::visit(BlockDim&) { return code("bdim"); }

CodeSeg IRPrinter2::visit(GridDim&) { return code("gdim"); }

CodeSeg IRPrinter2::visit(Loop& e)
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

CodeSeg IRPrinter2::visit(FetchDataPtr& e)
{
    return code(eval(e.vec), "->_" + to_string(e.col));
}

CodeSeg IRPrinter2::visit(NoOp&) { return code(""); }

CodeSeg IRPrinter2::visit(Define& define)
{
    emit(nl(), define.sym->name, " = ", eval(define.val));
    return code(define.sym->name);
}

CodeSeg IRPrinter2::visit(InitVal& init_val)
{
    for (auto init : init_val.inits) { emit(nl(), eval(init)); }
    return eval(init_val.val);
}

CodeSeg IRPrinter2::visit(Func& fn)
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

string IRPrinter2::Build(Expr expr)
{
    IRPrinter2 printer2(make_unique<IREmitterCtx>());
    return printer2.eval(expr)->to_string(-1);
}

string IRPrinter2::Build(llvm::Module& llmod)
{
    std::string str;
    llvm::raw_string_ostream ostr(str);
    ostr << llmod;
    ostr.flush();
    return ostr.str();
}
