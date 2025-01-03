#include "reffine/pass/base/visitor.h"

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
void NotNull::Accept(Visitor& v) { v.Visit(*this); }
void Reduce::Accept(Visitor& v) { v.Visit(*this); }
void Stmts::Accept(Visitor& v) { v.Visit(*this); }
void Alloc::Accept(Visitor& v) { v.Visit(*this); }
void Load::Accept(Visitor& v) { v.Visit(*this); }
void Store::Accept(Visitor& v) { v.Visit(*this); }
void Loop::Accept(Visitor& v) { v.Visit(*this); }
void IsValid::Accept(Visitor& v) { v.Visit(*this); }
void SetValid::Accept(Visitor& v) { v.Visit(*this); }
void FetchDataPtr::Accept(Visitor& v) { v.Visit(*this); }
void NoOp::Accept(Visitor& v) { v.Visit(*this); }
