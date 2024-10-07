#include "reffine/pass/visitor.h"

using namespace reffine;

void SymNode::Accept(Visitor& v) const { v.Visit(*this); }
void Func::Accept(Visitor& v) const { v.Visit(*this); }
void Call::Accept(Visitor& v) const { v.Visit(*this); }
void Select::Accept(Visitor& v) const { v.Visit(*this); }
void IfElse::Accept(Visitor& v) const { v.Visit(*this); }
void Exists::Accept(Visitor& v) const { v.Visit(*this); }
void Const::Accept(Visitor& v) const { v.Visit(*this); }
void Cast::Accept(Visitor& v) const { v.Visit(*this); }
void NaryExpr::Accept(Visitor& v) const { v.Visit(*this); }
void Read::Accept(Visitor& v) const { v.Visit(*this); }
void Write::Accept(Visitor& v) const { v.Visit(*this); }
void Stmts::Accept(Visitor& v) const { v.Visit(*this); }
void Alloc::Accept(Visitor& v) const { v.Visit(*this); }
void Load::Accept(Visitor& v) const { v.Visit(*this); }
void Store::Accept(Visitor& v) const { v.Visit(*this); }
void Loop::Accept(Visitor& v) const { v.Visit(*this); }
void IsValid::Accept(Visitor& v) const { v.Visit(*this); }
void SetValid::Accept(Visitor& v) const { v.Visit(*this); }
void FetchDataPtr::Accept(Visitor& v) const { v.Visit(*this); }
void NoOp::Accept(Visitor& v) const { v.Visit(*this); }
