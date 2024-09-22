#include <unordered_set>

#include "reffine/pass/printer.h"

using namespace reffine;
using namespace std;

static const auto EXISTS = "\u2203";
//static const auto FORALL = "\u2200";
//static const auto IN = "\u2208";
//static const auto PHI = "\u0278";

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
    ostr << "(" << e.type.str() << ") ";
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

void IRPrinter::Visit(const Func& fn)
{
    ostr << "def " << fn.name << "(";
    for (const auto& input : fn.inputs) {
        ostr << input->name << ", ";
    }
    ostr << (fn.inputs.size() > 0 ? "\b\b" : "") << ") {";

    enter_block();
    for (const auto& [sym, val] : fn.tbl) {
        emitassign(sym, val);
        emitnewline();
    }

    emitnewline();
    ostr << "return ";
    fn.output->Accept(*this);
    exit_block();

    ostr << "}";
    emitnewline();
}

void IRPrinter::Visit(const Stmts& stmts)
{
    for (const auto& stmt : stmts.stmts) {
        stmt->Accept(*this);
        emitnewline();
    }
}

void IRPrinter::Visit(const Alloc& alloc)
{
    ostr << "alloc " << alloc.type.deref().str();
}

void IRPrinter::Visit(const Load& load)
{
    emitfunc("load", vector<Expr>{load.addr});
}

void IRPrinter::Visit(const Store& store)
{
    emitfunc("store", vector<Expr>{store.addr, store.val});
}

void IRPrinter::Visit(const Loop& loop)
{
    ostr << "{";
    enter_block();

    emitcomment("initialization");
    emitnewline();
    loop.init->Accept(*this);
    emitnewline();

    ostr << "while(1) {";
    enter_block();

    emitcomment("exit condition check");
    emitnewline();
    ostr << "if (!";
    loop.exit_cond->Accept(*this);
    ostr << ") break";
    emitnewline();
    emitnewline();

    if (loop.body_cond) {
        emitcomment("body condition check");
        emitnewline();
        ostr << "if (!";
        loop.body_cond->Accept(*this);
        ostr << ") break";
        emitnewline();
        emitnewline();
    }

    emitcomment("loop body");
    emitnewline();
    loop.body->Accept(*this);

    if (loop.incr) {
        emitnewline();
        emitcomment("update indices");
        emitnewline();
        loop.incr->Accept(*this);
    }

    exit_block();
    ostr << "}";
    emitnewline();
    emitnewline();

    ostr << "return ";
    loop.output->Accept(*this);
    exit_block();
    ostr << "}";
}

string IRPrinter::Build(const Stmt stmt)
{
    IRPrinter printer;
    stmt->Accept(printer);
    return printer.ostr.str();
}

string IRPrinter::Build(const llvm::Module& llmod)
{
    std::string str;
    llvm::raw_string_ostream ostr(str);
    ostr << llmod;
    ostr.flush();
    return ostr.str();
}
