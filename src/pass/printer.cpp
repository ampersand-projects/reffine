#include "reffine/pass/printer.h"

#include <unordered_set>

#include "reffine/builder/reffiner.h"
#include "reffine/pass/symanalysis.h"

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

void IRPrinter::Visit(StmtExprNode& expr)
{
    ostr << "(void)[";
    expr.stmt->Accept(*this);
    ostr << "]";
}

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

    auto syminfo_map = SymAnalysis::Build(std::make_shared<Func>(fn));

    std::vector<std::pair<Sym, SymInfo>> ordered_symbols;
    for (const auto& [sym, info] : syminfo_map) {
        ordered_symbols.emplace_back(sym, info);
    }

    std::sort(ordered_symbols.begin(), ordered_symbols.end(),
              [](const auto& x, const auto& y) {
                  return x.second.order < y.second.order;
              });

    enter_block();
    for (const auto& [sym, info] : ordered_symbols) {
        auto it = fn.tbl.find(sym);
        if (it != fn.tbl.end()) {
            emitassign(sym, it->second);
            emitnewline();
        }
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

void IRPrinter::Visit(AtomicOp& e)
{
    switch (e.op) {
        case MathOp::ADD:
            emitfunc("atomic_add", {e.addr, e.val});
            break;
        case MathOp::SUB:
            emitfunc("atomic_sub", {e.addr, e.val});
            break;
        case MathOp::MAX:
            emitfunc("atomic_max", {e.addr, e.val});
            break;
        case MathOp::MIN:
            emitfunc("atomic_min", {e.addr, e.val});
            break;
        case MathOp::AND:
            emitfunc("atomic_and", {e.addr, e.val});
            break;
        case MathOp::OR:
            emitfunc("atomic_or", {e.addr, e.val});
            break;
        default:
            throw std::runtime_error("Invalid atomic operation");
            break;
    }
}

void IRPrinter::Visit(StructGEP& gep)
{
    emitfunc("structgep", vector<Expr>{gep.addr, _idx(gep.col)});
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

void IRPrinter::Visit(Length& len)
{
    emitfunc("length", {len.vec});
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
