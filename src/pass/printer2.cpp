#include "reffine/pass/printer2.h"

using namespace reffine;

static const auto FORALL = "\u2200";
//static const auto REDCLE = "\u2295";
//static const auto PHI = "\u0278";

CodeSeg IRPrinter2::visit(Sym sym)
{
    auto lhs = _line(sym->name);

    if (this->ctx().in_sym_tbl.find(sym) != this->ctx().in_sym_tbl.end()) {
        auto rhs = eval(this->ctx().in_sym_tbl.at(sym));
        emit(_nl(), lhs, " = ", rhs);
    }

    return lhs;
}

CodeSeg IRPrinter2::visit(StmtExprNode& e)
{
    return eval(e.stmt);
}

CodeSeg IRPrinter2::visit(Stmts& s)
{
    for (auto& stmt : s.stmts) {
        eval(stmt);
    }
    return _nl();
}

CodeSeg IRPrinter2::visit(Call& call)
{
    return _line("nary");
}

CodeSeg IRPrinter2::visit(IfElse&)
{
    return _line("nary");
}

CodeSeg IRPrinter2::visit(Select&)
{
    return _line("sel");
}

CodeSeg IRPrinter2::visit(Const& cnst)
{
    switch (cnst.type.btype) {
        case BaseType::BOOL:
            return _line(cnst.val ? "true" : "false");
        case BaseType::INT8:
        case BaseType::INT16:
        case BaseType::INT32:
        case BaseType::INT64:
            return _line(to_string((int64_t)cnst.val) + "i");
        case BaseType::UINT8:
        case BaseType::UINT16:
        case BaseType::UINT32:
        case BaseType::UINT64:
            return _line(to_string((int64_t)cnst.val) + "i");
        case BaseType::FLOAT32:
        case BaseType::FLOAT64:
            return _line(to_string((int64_t)cnst.val) + "i");
        case BaseType::IDX:
            return _line(to_string((int64_t)cnst.val) + "i");
        default:
            throw std::runtime_error("Invalid constant type");
    }
}

CodeSeg IRPrinter2::visit(Cast&)
{
    return _line("cast");
}

CodeSeg IRPrinter2::visit(Get& e)
{
    return _line("(", eval(e.val), ")._", to_string(e.col));
}

CodeSeg IRPrinter2::visit(New&)
{
    return _line("new");
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
            return _line("|", eval(e.arg(0)), "|");
        case MathOp::NEG:
            return _line("-", eval(e.arg(0)));
        case MathOp::SQRT:
            return code_func("min", {e.arg(0)});
        case MathOp::POW:
            return code_binary(e.arg(0), "^", e.arg(1));
        case MathOp::CEIL:
            return code_func("ceil", {e.arg(0)});
        case MathOp::FLOOR:
            return code_func("floor", {e.arg(0)});
        case MathOp::EQ:
            return code_binary(e.arg(0), "==", e.arg(1));
        case MathOp::NOT:
            return _line("!", eval(e.arg(0)));
        case MathOp::AND:
            return code_binary(e.arg(0), "&", e.arg(1));
        case MathOp::OR:
            return code_binary(e.arg(0), "|", e.arg(1));
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

CodeSeg IRPrinter2::visit(Op& op)
{
    auto pred_code = eval(op.pred);

    auto parent = enter_block();
    emit(FORALL, " ");
    for (const auto& iter : op.iters) { emit(iter->name, ", "); }

    emit(": [", pred_code, "] {");

    auto child = enter_block();
    vector<CodeSeg> outs_code;
    for (const auto& output : op.outputs) {
        auto out_code = eval(output);
        outs_code.push_back(out_code);
    }
    emit(_nl(), "return {");
    for (const auto& out_code : outs_code) {
        emit(out_code);
    }
    emit("}");
    auto op_body = exit_block(child);
    emit(op_body);

    emit(_nl(), "}");
    exit_block(parent);

    return child;
}

CodeSeg IRPrinter2::visit(Element& elem)
{
    auto line = _line(eval(elem.vec), "[");
    for (const auto& iter : elem.iters) {
        line->emit(eval(iter), ", ");
    }
    line->emit("]");

    return line;
}

CodeSeg IRPrinter2::visit(NotNull& e)
{
    return _line("~(", eval(e.elem), ")");
}

CodeSeg IRPrinter2::visit(Reduce&)
{
    return _line("red");
}

CodeSeg IRPrinter2::visit(Alloc&)
{
    return _line("alloc");
}

CodeSeg IRPrinter2::visit(Load&)
{
    return _line("load");
}

CodeSeg IRPrinter2::visit(Store&)
{
    return _line("stroe");
}

CodeSeg IRPrinter2::visit(AtomicOp&)
{
    return _line("atom");
}

CodeSeg IRPrinter2::visit(StructGEP&)
{
    return _line("gep");
}

CodeSeg IRPrinter2::visit(ThreadIdx&)
{
    return _line("tidx");
}

CodeSeg IRPrinter2::visit(BlockIdx&)
{
    return _line("bid");
}

CodeSeg IRPrinter2::visit(BlockDim&)
{
    return _line("bdim");
}

CodeSeg IRPrinter2::visit(GridDim&)
{
    return _line("gdim");
}

CodeSeg IRPrinter2::visit(Loop&)
{
    return _line("loop");
}

CodeSeg IRPrinter2::visit(Lookup&)
{
    return _line("lookup");
}

CodeSeg IRPrinter2::visit(MakeVector&)
{
    return _line("make");
}

CodeSeg IRPrinter2::visit(FetchDataPtr&)
{
    return _line("fetch");
}

CodeSeg IRPrinter2::visit(NoOp&)
{
    return _line("noop");
}

void IRPrinter2::visit(Func& fn)
{
    emit("def ", fn.name, "(");
    for (auto& input : fn.inputs) { emit(input->name, ", "); }
    emit(") {");

    auto parent = enter_block();
    emit(_nl(), "return ", eval(fn.output));
    auto child = exit_block(parent);
    emit(child);
    emit(_nl(), "}");
}

string IRPrinter2::Build(shared_ptr<Func> func)
{
    IRPrinter2Ctx ctx(func);
    IRPrinter2 printer2(ctx);
    func->Accept(printer2);
    return printer2._block->to_string(-1);
}
