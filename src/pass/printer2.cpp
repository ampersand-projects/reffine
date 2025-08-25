#include "reffine/pass/printer2.h"

using namespace reffine;

//static const auto FORALL = "\u2200";
//static const auto REDCLE = "\u2295";
//static const auto AND = "\u2227";
//static const auto OR = "\u2228";
//static const auto PHI = "\u0278";

CodeSeg IRPrinter2::visit(Sym sym)
{
    return _str(sym->name);
}

CodeSeg IRPrinter2::visit(StmtExprNode&)
{
    return _str("nary");
}

CodeSeg IRPrinter2::visit(Stmts&)
{
    return _str("nary");
}

CodeSeg IRPrinter2::visit(Call&)
{
    return _str("nary");
}

CodeSeg IRPrinter2::visit(IfElse&)
{
    return _str("nary");
}

CodeSeg IRPrinter2::visit(Select&)
{
    return _str("sel");
}

CodeSeg IRPrinter2::visit(Const&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(Cast&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(Get&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(New&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(NaryExpr&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(Op&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(Element&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(NotNull&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(Reduce&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(Alloc&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(Load&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(Store&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(AtomicOp&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(StructGEP&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(ThreadIdx&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(BlockIdx&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(BlockDim&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(GridDim&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(Loop&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(Lookup&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(MakeVector&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(FetchDataPtr&)
{
    return _str("const");
}

CodeSeg IRPrinter2::visit(NoOp&)
{
    return _str("const");
}

void IRPrinter2::visit(Func& fn)
{
    emit("def ", fn.name, "(");
    for (auto& input : fn.inputs) { emit(input->name, ", "); }
    emit(")");

    auto child = _blk();
    auto parent = enter_block(child);
    emit("return ", eval(fn.output));
    exit_block(parent);
    emit(child);
    emit(_nl(), "}");

}

string IRPrinter2::Build(shared_ptr<Func> func)
{
    IRPrinter2Ctx ctx(func);
    IRPrinter2 printer2(ctx);
    func->Accept(printer2);
    return printer2._block->to_string(-1);
}
