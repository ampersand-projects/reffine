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
    IRPass::Visit(sym);
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
    eval(e.arg);
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
        case MathOp::ABS: ostr << "|"; eval(e.arg(0)); ostr << "|"; break;
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
    emitfunc("read<" + std::to_string(read.col) + ">", { read.vec, read.idx });
}

void IRPrinter::Visit(const Write& write)
{
    emitfunc("write<" + std::to_string(write.col) + ">", { write.vec, write.idx, write.val });
}

void IRPrinter::Visit(const Call& call)
{
    emitfunc(call.name, call.args);
}

void IRPrinter::Visit(const IfElse& ifelse)
{
    ostr << "if (";
    eval(ifelse.cond);
    ostr << ") {";

    enter_block();
    eval(ifelse.true_body);
    exit_block();

    ostr << "} else {";
    enter_block();
    eval(ifelse.false_body);
    exit_block();
    ostr << "}";

    emitnewline();
}

void IRPrinter::Visit(const NoOp&)
{
    ostr << "noop";
}

void IRPrinter::Visit(const Select& select)
{
    ostr << "(";
    eval(select.cond);
    ostr << " ? ";
    eval(select.true_body);
    ostr << " : ";
    eval(select.false_body);
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
    eval(fn.output);
    exit_block();

    ostr << "}";
    emitnewline();
}

void IRPrinter::Visit(const Stmts& stmts)
{
    for (const auto& stmt : stmts.stmts) {
        eval(stmt);
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

    if (loop.init) {
        emitcomment("initialization");
        emitnewline();
        eval(loop.init);
        emitnewline();
    }

    ostr << "while(1) {";
    enter_block();

    emitcomment("exit condition check");
    emitnewline();
    ostr << "if (";
    eval(loop.exit_cond);
    ostr << ") break";
    emitnewline();
    emitnewline();

    if (loop.body_cond) {
        emitcomment("body condition check");
        emitnewline();
        ostr << "if (!";
        eval(loop.body_cond);
        ostr << ") break";
        emitnewline();
        emitnewline();
    }

    emitcomment("loop body");
    emitnewline();
    eval(loop.body);

    if (loop.incr) {
        emitnewline();
        emitcomment("update indices");
        emitnewline();
        eval(loop.incr);
    }

    exit_block();
    ostr << "}";
    emitnewline();
    emitnewline();

    if (loop.post) {
        emitcomment("post processing");
        emitnewline();
        eval(loop.post);
        emitnewline();
    }

    emitnewline();
    ostr << "return ";
    eval(loop.output);
    exit_block();
    ostr << "}";
}

void IRPrinter::Visit(const IsValid& is_valid)
{
    emitfunc("is_valid<" + std::to_string(is_valid.col) + ">", { is_valid.vec, is_valid.idx });
}

void IRPrinter::Visit(const SetValid& set_valid)
{
    emitfunc("set_valid<" + std::to_string(set_valid.col) + ">", { set_valid.vec, set_valid.idx, set_valid.validity });
}

void IRPrinter::Visit(const FetchDataPtr& fetch_data_ptr)
{
    emitfunc("fetch_data_ptr<" + std::to_string(fetch_data_ptr.col) + ">", { fetch_data_ptr.vec, fetch_data_ptr.idx });
}

string IRPrinter::Build(const shared_ptr<Func> func)
{
    IRPrinter printer(func);
    func->Accept(printer);
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
