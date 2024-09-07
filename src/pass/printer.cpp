#include <unordered_set>

#include "reffine/pass/printer.h"

using namespace reffine;
using namespace std;

static const auto EXISTS = "\u2203";
//static const auto FORALL = "\u2200";
//static const auto IN = "\u2208";
//static const auto PHI = "\u0278";

string idx_str(int64_t idx)
{
    ostringstream ostr;
    if (idx > 0) {
        ostr << " + " << idx;
    } else if (idx < 0) {
        ostr << " - " << -idx;
    }
    return ostr.str();
}

void IRPrinter::Visit(const SymNode& sym)
{
    ostr << sym.name;
}

void IRPrinter::Visit(const Exists& exists)
{
    emitunary(EXISTS, exists.sym);
}

void IRPrinter::Visit(const Const& cnst)
{
    switch (cnst.type.btype) {
        case BaseType::BOOL: ostr << (cnst.val ? "true" : "false"); break;
        case BaseType::INT8:
        case BaseType::INT16:
        case BaseType::INT32:
        case BaseType::INT64: ostr << cnst.val << "i"; break;
        case BaseType::UINT8:
        case BaseType::UINT16:
        case BaseType::UINT32:
        case BaseType::UINT64: ostr << cnst.val << "u"; break;
        case BaseType::FLOAT32:
        case BaseType::FLOAT64: ostr << cnst.val << "f"; break;
        case BaseType::IDX: ostr << cnst.val << "x"; break;
        default: throw std::runtime_error("Invalid constant type");
    }
}

void IRPrinter::Visit(const Cast& e)
{
    string destty;
    switch (e.type.btype) {
        case BaseType::INT8: destty = "int8"; break;
        case BaseType::INT16: destty = "int16"; break;
        case BaseType::INT32: destty = "int32"; break;
        case BaseType::INT64: destty = "long"; break;
        case BaseType::UINT8: destty = "uint8"; break;
        case BaseType::UINT16: destty = "uint16"; break;
        case BaseType::UINT32: destty = "uint32"; break;
        case BaseType::UINT64: destty = "ulong"; break;
        case BaseType::FLOAT32: destty = "float"; break;
        case BaseType::FLOAT64: destty = "double"; break;
        case BaseType::BOOL: destty = "bool"; break;
        case BaseType::IDX: destty = "idx"; break;
        default: throw std::runtime_error("Invalid destination type for cast");
    }

    ostr << "(" << destty << ") ";
    e.arg->Accept(*this);
}

void IRPrinter::Visit(const NaryExpr& e)
{
    switch (e.op) {
        case MathOp::ADD: emitbinary(e.arg(0), "+", e.arg(1)); break;
        case MathOp::SUB: emitbinary(e.arg(0), "-", e.arg(1)); break;
        case MathOp::MUL: emitbinary(e.arg(0), "*", e.arg(1)); break;
        case MathOp::DIV: emitbinary(e.arg(0), "/", e.arg(1)); break;
        case MathOp::MAX: emitfunc("max", {e.arg(0), e.arg(1)}); break;
        case MathOp::MIN: emitfunc("min", {e.arg(0), e.arg(1)}); break;
        case MathOp::MOD: emitbinary(e.arg(0), "%", e.arg(1)); break;
        case MathOp::ABS: ostr << "|"; e.arg(0)->Accept(*this); ostr << "|"; break;
        case MathOp::NEG: emitunary("-", {e.arg(0)}); break;
        case MathOp::SQRT: emitfunc("sqrt", {e.arg(0)}); break;
        case MathOp::POW: emitfunc("pow", {e.arg(0), e.arg(1)}); break;
        case MathOp::CEIL: emitfunc("ceil", {e.arg(0)}); break;
        case MathOp::FLOOR: emitfunc("floor", {e.arg(0)}); break;
        case MathOp::EQ: emitbinary(e.arg(0), "==", e.arg(1)); break;
        case MathOp::NOT: emitunary("!", e.arg(0)); break;
        case MathOp::AND: emitbinary(e.arg(0), "&&", e.arg(1)); break;
        case MathOp::OR: emitbinary(e.arg(0), "||", e.arg(1)); break;
        case MathOp::LT: emitbinary(e.arg(0), "<", e.arg(1)); break;
        case MathOp::LTE: emitbinary(e.arg(0), "<=", e.arg(1)); break;
        case MathOp::GT: emitbinary(e.arg(0), ">", e.arg(1)); break;
        case MathOp::GTE: emitbinary(e.arg(0), ">=", e.arg(1)); break;
        default: throw std::runtime_error("Invalid math operation");
    }
}

void IRPrinter::Visit(const Read& read)
{
    emitfunc("read", { read.vector, read.idx });
}

void IRPrinter::Visit(const PushBack& push_back)
{
    emitfunc("push_back", { push_back.vector, push_back.val });
}

void IRPrinter::Visit(const Call& call)
{
    emitfunc(call.name, call.args);
}

void IRPrinter::Visit(const IfElse& ifelse)
{
    ifelse.cond->Accept(*this);
    ostr << " ? ";
    ifelse.true_body->Accept(*this);
    ostr << " : ";
    ifelse.false_body->Accept(*this);
}

void IRPrinter::Visit(const Select& select)
{
    ostr << "(";
    select.cond->Accept(*this);
    ostr << " ? ";
    select.true_body->Accept(*this);
    ostr << " : ";
    select.false_body->Accept(*this);
    ostr << ")";
}

void IRPrinter::Visit(const Loop& loop)
{
    emitcomment("Loop block");
    emitnewline();
    ostr << "{";
    enter_block();

    emitcomment("initialization");
    emitnewline();
    for (const auto& [idx, init_expr] : loop.idx_inits) {
        emitassign(idx, init_expr);
        emitnewline();
    }
    emitnewline();

    ostr << "while(1) {";
    enter_block();

    emitcomment("exit condition check");
    emitnewline();
    ostr << "if (";
    loop.exit_cond->Accept(*this);
    ostr << ") break;";
    emitnewline();
    emitnewline();

    emitcomment("body condition check");
    emitnewline();
    ostr << "if (";
    loop.body_cond->Accept(*this);
    ostr << ") break;";
    emitnewline();
    emitnewline();

    emitcomment("loop body");
    emitnewline();
    loop.body->Accept(*this);
    emitnewline();
    emitnewline();

    emitcomment("update indices");
    for (const auto& [idx, incr_expr] : loop.idx_incrs) {
        emitnewline();
        emitassign(idx, incr_expr);
    }

    exit_block();
    ostr << "}";
    emitnewline();
    emitnewline();

    ostr << "return ";
    loop.output->Accept(*this);
    exit_block();
    ostr << "}";
    emitnewline();
}

string IRPrinter::Build(const Stmt stmt)
{
    IRPrinter printer;
    stmt->Accept(printer);
    return printer.ostr.str();
}

string IRPrinter::Build(const llvm::Module* mod)
{
    std::string str;
    llvm::raw_string_ostream ostr(str);
    ostr << *mod;
    ostr.flush();
    return ostr.str();
}
