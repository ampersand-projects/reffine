#include <sstream>

#include "reffine/pass/base/visitor.h"
#include "reffine/pass/printer2.h"

using namespace reffine;

void SymNode::Accept(Visitor& v) { v.Visit(*this); }
void Func::Accept(Visitor& v) { v.Visit(*this); }
void Call::Accept(Visitor& v) { v.Visit(*this); }
void Select::Accept(Visitor& v) { v.Visit(*this); }
void IfElse::Accept(Visitor& v) { v.Visit(*this); }
void Const::Accept(Visitor& v) { v.Visit(*this); }
void Cast::Accept(Visitor& v) { v.Visit(*this); }
void Get::Accept(Visitor& v) { v.Visit(*this); }
void New::Accept(Visitor& v) { v.Visit(*this); }
void NaryExpr::Accept(Visitor& v) { v.Visit(*this); }
void Op::Accept(Visitor& v) { v.Visit(*this); }
void Element::Accept(Visitor& v) { v.Visit(*this); }
void Lookup::Accept(Visitor& v) { v.Visit(*this); }
void In::Accept(Visitor& v) { v.Visit(*this); }
void Reduce::Accept(Visitor& v) { v.Visit(*this); }
void Stmts::Accept(Visitor& v) { v.Visit(*this); }
void Alloc::Accept(Visitor& v) { v.Visit(*this); }
void Load::Accept(Visitor& v) { v.Visit(*this); }
void Store::Accept(Visitor& v) { v.Visit(*this); }
void AtomicOp::Accept(Visitor& v) { v.Visit(*this); }
void StructGEP::Accept(Visitor& v) { v.Visit(*this); }
void ThreadIdx::Accept(Visitor& v) { v.Visit(*this); }
void BlockIdx::Accept(Visitor& v) { v.Visit(*this); }
void BlockDim::Accept(Visitor& v) { v.Visit(*this); }
void GridDim::Accept(Visitor& v) { v.Visit(*this); }
void Loop::Accept(Visitor& v) { v.Visit(*this); }
void FetchDataPtr::Accept(Visitor& v) { v.Visit(*this); }
void NoOp::Accept(Visitor& v) { v.Visit(*this); }
void Define::Accept(Visitor& v) { v.Visit(*this); }
void InitVal::Accept(Visitor& v) { v.Visit(*this); }
void ReadData::Accept(Visitor& v) { v.Visit(*this); }
void WriteData::Accept(Visitor& v) { v.Visit(*this); }
void ReadBit::Accept(Visitor& v) { v.Visit(*this); }
void WriteBit::Accept(Visitor& v) { v.Visit(*this); }
void ReadRunEnd::Accept(Visitor& v) { v.Visit(*this); }
void Length::Accept(Visitor& v) { v.Visit(*this); }
void SubVector::Accept(Visitor& v) { v.Visit(*this); }

Sym ExprNode::symify(string prefix)
{
    auto* addr = static_cast<void*>(this);
    std::stringstream ss;
    ss << prefix << "_" << addr;
    return make_shared<SymNode>(ss.str(), this->type);
}

string ExprNode::str()
{
    Expr expr(const_cast<ExprNode*>(this), [](ExprNode*) {});
    return IRPrinter2::Build(expr);
}
