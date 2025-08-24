#include "reffine/pass/clanggen.h"

using namespace std;
using namespace llvm;
using namespace reffine;

CodeSeg ClangGen::visit(Sym old_sym)
{

}

CodeSeg ClangGen::visit(Call&) {}
CodeSeg ClangGen::visit(IfElse&) {}
CodeSeg ClangGen::visit(NoOp&) {}
CodeSeg ClangGen::visit(Select&) {}
CodeSeg ClangGen::visit(Const&) {}
CodeSeg ClangGen::visit(Get&) {}
CodeSeg ClangGen::visit(Cast&) {}
CodeSeg ClangGen::visit(NaryExpr&) {}
CodeSeg ClangGen::visit(Stmts&) {}
CodeSeg ClangGen::visit(Alloc&) {}
CodeSeg ClangGen::visit(Load&) {}
CodeSeg ClangGen::visit(Store&) {}
CodeSeg ClangGen::visit(AtomicOp&) {}
CodeSeg ClangGen::visit(StructGEP&) {}
CodeSeg ClangGen::visit(ThreadIdx&) {}
CodeSeg ClangGen::visit(BlockIdx&) {}
CodeSeg ClangGen::visit(BlockDim&) {}
CodeSeg ClangGen::visit(GridDim&) {}
CodeSeg ClangGen::visit(Loop&) {}
CodeSeg ClangGen::visit(FetchDataPtr&) {}
void ClangGen::visit(Func&) {}


void ClangGen::Build(shared_ptr<Func> func, Module& llmod)
{

}
