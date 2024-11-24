#include <unordered_set>

#include "reffine/pass/printer.h"

using namespace reffine;
using namespace std;

//static const auto EXISTS = "\u2203";
static const auto FORALL = "\u2200";
static const auto REDCLE = "\u2295";
//static const auto IN = "\u2208";
//static const auto PHI = "\u0278";

void IRPrinter::Visit(SymNode& sym)
{
    ostr << sym.name;
}

void IRPrinter::Visit(Const& cnst)
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

void IRPrinter::Visit(Cast& e)
{
    ostr << "(" << e.type.str() << ") ";
    e.arg->Accept(*this);
}

void IRPrinter::Visit(Get& e)
{
    emitfunc("get<" + std::to_string(e.col) + ">", { e.val });
}

void IRPrinter::Visit(NaryExpr& e)
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

void IRPrinter::Visit(Op& op)
{
    ostr << FORALL << " ";
    for (const auto& idx : op.idxs) {
        ostr << idx->name << ", ";
    }
    if (op.idxs.size() > 0) { ostr << "\b\b"; }
    ostr << ": ";

    ostr << "(";
    for (const auto& pred : op.preds) {
        pred->Accept(*this);
        ostr << " && ";
    }
    ostr << "\b\b\b\b";
    ostr << ") ";

    ostr << "{";
    for (const auto& output : op.outputs) {
        output->Accept(*this);
        ostr << ", ";
    }
    ostr << "\b\b";
    ostr << "}";
}

void IRPrinter::Visit(Element& elem)
{
    elem.vec->Accept(*this);
    ostr << "[";
    for (const auto& idx : elem.idxs) {
        idx->Accept(*this);
        ostr << ", ";
    }
    ostr << "\b\b";
    ostr << "]";
}

void IRPrinter::Visit(Reduce& red)
{
    auto init_val = red.init();
    auto val = make_shared<SymNode>("val", red.op.type.valty());
    auto state = make_shared<SymNode>("state", init_val->type);
    auto state2 = red.acc(state, val);

    ostr << REDCLE << " {";
    enter_block();
    red.op.Accept(*this);
    ostr << ", ";
    emitnewline();

    ostr << "state <- ";
    init_val->Accept(*this);
    ostr << ", ";
    emitnewline();

    ostr << "state <- ";
    state2->Accept(*this);

    exit_block();
    ostr << "}";
}

void IRPrinter::Visit(Call& call)
{
    emitfunc(call.name, call.args);
}

void IRPrinter::Visit(IfElse& ifelse)
{
    ostr << "if (";
    ifelse.cond->Accept(*this);
    ostr << ") {";

    enter_block();
    ifelse.true_body->Accept(*this);
    exit_block();

    ostr << "} else {";
    enter_block();
    ifelse.false_body->Accept(*this);
    exit_block();
    ostr << "}";

    emitnewline();
}

void IRPrinter::Visit(NoOp&)
{
    ostr << "noop";
}

void IRPrinter::Visit(Select& select)
{
    ostr << "(";
    select.cond->Accept(*this);
    ostr << " ? ";
    select.true_body->Accept(*this);
    ostr << " : ";
    select.false_body->Accept(*this);
    ostr << ")";
}

void IRPrinter::Visit(Func& fn)
{
    ostr << "def " << fn.name << "(";
    for (auto& input : fn.inputs) {
        ostr << input->name << ", ";
    }
    ostr << (fn.inputs.size() > 0 ? "\b\b" : "") << ") {";

    enter_block();
    for (auto& [sym, val] : fn.tbl) {
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

void IRPrinter::Visit(Stmts& stmts)
{
    for (auto& stmt : stmts.stmts) {
        stmt->Accept(*this);
        emitnewline();
    }
}

void IRPrinter::Visit(Alloc& alloc)
{
    ostr << "alloc " << alloc.type.deref().str();
}

void IRPrinter::Visit(Load& load)
{
    emitfunc("load", vector<Expr>{load.addr});
}

void IRPrinter::Visit(Store& store)
{
    emitfunc("store", vector<Expr>{store.addr, store.val});
}

void IRPrinter::Visit(Loop& loop)
{
    ostr << "{";
    enter_block();

    if (loop.init) {
        emitcomment("initialization");
        emitnewline();
        loop.init->Accept(*this);
        emitnewline();
    }

    ostr << "while(1) {";
    enter_block();

    emitcomment("exit condition check");
    emitnewline();
    ostr << "if (";
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

    if (loop.post) {
        emitcomment("post processing");
        emitnewline();
        loop.post->Accept(*this);
        emitnewline();
    }

    emitnewline();
    ostr << "return ";
    loop.output->Accept(*this);
    exit_block();
    ostr << "}";
}

void IRPrinter::Visit(IsValid& is_valid)
{
    emitfunc("is_valid<" + std::to_string(is_valid.col) + ">", { is_valid.vec, is_valid.idx });
}

void IRPrinter::Visit(SetValid& set_valid)
{
    emitfunc("set_valid<" + std::to_string(set_valid.col) + ">", { set_valid.vec, set_valid.idx, set_valid.validity });
}

void IRPrinter::Visit(FetchDataPtr& fetch_data_ptr)
{
    emitfunc("fetch_data_ptr<" + std::to_string(fetch_data_ptr.col) + ">", { fetch_data_ptr.vec, fetch_data_ptr.idx });
}

string IRPrinter::Build(Stmt stmt)
{
    IRPrinter printer;
    stmt->Accept(printer);
    return printer.ostr.str();
}

string IRPrinter::Build(llvm::Module& llmod)
{
    std::string str;
    llvm::raw_string_ostream ostr(str);
    ostr << llmod;
    ostr.flush();
    return ostr.str();
}
