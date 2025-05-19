#include "reffine/pass/printer.h"

#include <unordered_set>

#include "reffine/builder/reffiner.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

static const auto FORALL = "\u2200";
static const auto REDCLE = "\u2295";
static const auto AND = "\u2227";
static const auto OR = "\u2228";
static const auto PHI = "\u0278";
// static const auto IN = "\u2208";

void IRPrinter::Visit(SymNode& sym) { ostr << sym.name; }

void IRPrinter::Visit(StmtExprNode& expr) { expr.stmt->Accept(*this); }

void IRPrinter::Visit(Const& cnst)
{
    switch (cnst.type.btype) {
        case BaseType::BOOL:
            ostr << (cnst.val ? "true" : "false");
            break;
        case BaseType::INT8:
        case BaseType::INT16:
        case BaseType::INT32:
        case BaseType::INT64:
            ostr << (int64_t)cnst.val << "i";
            break;
        case BaseType::UINT8:
        case BaseType::UINT16:
        case BaseType::UINT32:
        case BaseType::UINT64:
            ostr << (uint64_t)cnst.val << "u";
            break;
        case BaseType::FLOAT32:
        case BaseType::FLOAT64:
            ostr << cnst.val << "f";
            break;
        case BaseType::IDX:
            ostr << (uint64_t)cnst.val << "x";
            break;
        default:
            throw std::runtime_error("Invalid constant type");
    }
}

void IRPrinter::Visit(Cast& e)
{
    ostr << "(" << e.type.str() << ") ";
    e.arg->Accept(*this);
}

void IRPrinter::Visit(Get& e)
{
    ostr << "(";
    e.val->Accept(*this);
    ostr << ")._" << e.col;
}

void IRPrinter::Visit(New& e)
{
    ostr << "{";
    for (const auto& val : e.vals) {
        val->Accept(*this);
        ostr << ", ";
    }
    ostr << "}";
}

void IRPrinter::Visit(NaryExpr& e)
{
    switch (e.op) {
        case MathOp::ADD:
            emitbinary(e.arg(0), "+", e.arg(1));
            break;
        case MathOp::SUB:
            emitbinary(e.arg(0), "-", e.arg(1));
            break;
        case MathOp::MUL:
            emitbinary(e.arg(0), "*", e.arg(1));
            break;
        case MathOp::DIV:
            emitbinary(e.arg(0), "/", e.arg(1));
            break;
        case MathOp::MAX:
            emitfunc("max", {e.arg(0), e.arg(1)});
            break;
        case MathOp::MIN:
            emitfunc("min", {e.arg(0), e.arg(1)});
            break;
        case MathOp::MOD:
            emitbinary(e.arg(0), "%", e.arg(1));
            break;
        case MathOp::ABS:
            ostr << "|";
            e.arg(0)->Accept(*this);
            ostr << "|";
            break;
        case MathOp::NEG:
            emitunary("-", {e.arg(0)});
            break;
        case MathOp::SQRT:
            emitfunc("sqrt", {e.arg(0)});
            break;
        case MathOp::POW:
            emitfunc("pow", {e.arg(0), e.arg(1)});
            break;
        case MathOp::CEIL:
            emitfunc("ceil", {e.arg(0)});
            break;
        case MathOp::FLOOR:
            emitfunc("floor", {e.arg(0)});
            break;
        case MathOp::EQ:
            emitbinary(e.arg(0), "==", e.arg(1));
            break;
        case MathOp::NOT:
            emitunary("!", e.arg(0));
            break;
        case MathOp::AND:
            emitbinary(e.arg(0), AND, e.arg(1));
            break;
        case MathOp::OR:
            emitbinary(e.arg(0), OR, e.arg(1));
            break;
        case MathOp::LT:
            emitbinary(e.arg(0), "<", e.arg(1));
            break;
        case MathOp::LTE:
            emitbinary(e.arg(0), "<=", e.arg(1));
            break;
        case MathOp::GT:
            emitbinary(e.arg(0), ">", e.arg(1));
            break;
        case MathOp::GTE:
            emitbinary(e.arg(0), ">=", e.arg(1));
            break;
        default:
            throw std::runtime_error("Invalid math operation");
    }
}

void IRPrinter::Visit(Op& op)
{
    ostr << FORALL << " ";
    for (const auto& iter : op.iters) { ostr << iter->name << ", "; }
    if (op.iters.size() > 0) { ostr << "\b\b"; }
    ostr << ": ";

    ostr << "(";
    if (op._lower) {
        ostr << " >= ";
        op._lower->Accept(*this);
        ostr << "; ";
    }
    if (op._upper) {
        ostr << " <= ";
        op._upper->Accept(*this);
        ostr << "; ";
    }
    if (op._incr) {
        ostr << " <- ";
        op._incr->Accept(*this);
        ostr << "; ";
    }
    op.pred->Accept(*this);
    ostr << ")";

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
    for (const auto& iter : elem.iters) {
        iter->Accept(*this);
        ostr << ", ";
    }
    ostr << "\b\b";
    ostr << "]";
}

void IRPrinter::Visit(NotNull& not_null)
{
    not_null.elem->Accept(*this);
    ostr << "!=" << PHI;
}

void IRPrinter::Visit(Reduce& red)
{
    auto init_val = red.init();
    auto val = _sym("val", red.op.type.valty());
    auto state = _sym("state", init_val->type);
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

void IRPrinter::Visit(Call& call) { emitfunc(call.name, call.args); }

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

void IRPrinter::Visit(NoOp&) { ostr << "noop"; }

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
    for (auto& input : fn.inputs) { ostr << input->name << ", "; }
    ostr << (fn.inputs.size() > 0 ? "\b\b" : "") << ") {";

    enter_block();
    for (auto& [sym, val] : fn.tbl) {
        emitassign(sym, val);
        emitnewline();
    }

    emitnewline();
    if (!fn.output->type.is_void()) { ostr << "return "; }
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

void IRPrinter::Visit(Load& load) { emitfunc("load", vector<Expr>{load.addr}); }

void IRPrinter::Visit(Store& store)
{
    emitfunc("store", vector<Expr>{store.addr, store.val});
}

void IRPrinter::Visit(ThreadIdx& tidx) { ostr << "tidx"; }

void IRPrinter::Visit(BlockIdx& bidx) { ostr << "bidx"; }

void IRPrinter::Visit(BlockDim& bdim) { ostr << "bdim"; }

void IRPrinter::Visit(GridDim& bdim) { ostr << "gdim"; }

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
    emitfunc("is_valid<" + std::to_string(is_valid.col) + ">",
             {is_valid.vec, is_valid.idx});
}

void IRPrinter::Visit(SetValid& set_valid)
{
    emitfunc("set_valid<" + std::to_string(set_valid.col) + ">",
             {set_valid.vec, set_valid.idx, set_valid.validity});
}

void IRPrinter::Visit(Lookup& lookup)
{
    emitfunc("lookup", {lookup.vec, lookup.idx});
}

void IRPrinter::Visit(Locate& locate)
{
    ostr << "locate(";
    locate.vec->Accept(*this);
    ostr << ", {";
    for (const auto& iter : locate.iters) {
        iter->Accept(*this);
        ostr << ", ";
    }
    ostr << "\b\b";
    ostr << ")";
}

void IRPrinter::Visit(FetchDataPtr& fetch_data_ptr)
{
    emitfunc("fetch_data_ptr<" + std::to_string(fetch_data_ptr.col) + ">",
             {fetch_data_ptr.vec, fetch_data_ptr.idx});
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

string IRPrinter2::visit(Sym sym)
{
    auto expr_str = eval(this->ctx().in_sym_tbl.at(sym));
    emitline(sym->name + ": " + sym->type.str() + " = " + expr_str);

    return sym->name;
}

string IRPrinter2::visit(StmtExprNode&) { return nullptr; }
string IRPrinter2::visit(Select&) { return nullptr; }
string IRPrinter2::visit(IfElse&) { return nullptr; }

string IRPrinter2::visit(Const& cnst)
{
    switch (cnst.type.btype) {
        case BaseType::BOOL:
            return (cnst.val ? "true" : "false");
        case BaseType::INT8:
        case BaseType::INT16:
        case BaseType::INT32:
        case BaseType::INT64:
            return to_string((int64_t) cnst.val);
        case BaseType::UINT8:
        case BaseType::UINT16:
        case BaseType::UINT32:
        case BaseType::UINT64:
        case BaseType::IDX:
            return to_string((uint64_t) cnst.val);
        case BaseType::FLOAT32:
        case BaseType::FLOAT64:
            return to_string(cnst.val);
        default:
            throw std::runtime_error("Invalid constant type");
    }
}

string IRPrinter2::visit(Cast&) { return nullptr; }
string IRPrinter2::visit(Get&) { return nullptr; }
string IRPrinter2::visit(New&) { return nullptr; }
string IRPrinter2::visit(NaryExpr&) { return nullptr; }
string IRPrinter2::visit(Op&) { return nullptr; }
string IRPrinter2::visit(Element&) { return nullptr; }
string IRPrinter2::visit(Lookup&) { return nullptr; }
string IRPrinter2::visit(Locate&) { return nullptr; }
string IRPrinter2::visit(NotNull&) { return nullptr; }
string IRPrinter2::visit(Reduce&) { return nullptr; }
string IRPrinter2::visit(Call&) { return nullptr; }
string IRPrinter2::visit(Stmts&) { return nullptr; }
string IRPrinter2::visit(Alloc&) { return nullptr; }
string IRPrinter2::visit(Load&) { return nullptr; }
string IRPrinter2::visit(Store&) { return nullptr; }
string IRPrinter2::visit(ThreadIdx&) { return nullptr; }
string IRPrinter2::visit(BlockIdx&) { return nullptr; }
string IRPrinter2::visit(BlockDim&) { return nullptr; }
string IRPrinter2::visit(GridDim&) { return nullptr; }
string IRPrinter2::visit(Loop&) { return nullptr; }
string IRPrinter2::visit(IsValid&) { return nullptr; }
string IRPrinter2::visit(SetValid&) { return nullptr; }
string IRPrinter2::visit(FetchDataPtr&) { return nullptr; }
string IRPrinter2::visit(NoOp&) { return nullptr; }

void IRPrinter2::visit(Func& func)
{
    emit("def " + func.name + "(");

    for (const auto& input : func.inputs) {
        emit(input->name + ": " + input->type.str() + ", ");
    }
    if (func.inputs.size() > 0) { emit("\b\b"); }
    emit(") {");

    IRPrinter2Ctx ctx(func.tbl);
    IRPrinter2 printer(ctx);

    auto output = printer.eval(func.output);
    printer.emitline();
    if (!func.output->type.is_void()) { printer.emit("return "); }
    printer.emit(output);

    this->_ctx.block->lines.push_back(printer._ctx.block);

    emitline("}");
}


string IRPrinter2::Build(Stmt stmt)
{
    IRPrinter2Ctx ctx;
    IRPrinter2 printer(ctx);
    stmt->Accept(printer);
    return ctx.block->to_string(-1);
}
