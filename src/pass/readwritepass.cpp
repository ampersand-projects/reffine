#include "reffine/pass/readwritepass.h"

#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

Expr ReadWritePass::visit(ReadData& expr)
{
    return _load(_fetch(eval(expr.vec), expr.col), eval(expr.idx));
}

Expr ReadWritePass::visit(WriteData& expr)
{
    auto ptr = _fetch(eval(expr.vec), expr.col);
    return _store(ptr, eval(expr.val), eval(expr.idx));
}

Expr ReadWritePass::visit(ReadBit& expr)
{
    return _isval(eval(expr.vec), eval(expr.idx), expr.col);
}

Expr ReadWritePass::visit(WriteBit& expr)
{
    return _setval(eval(expr.vec), eval(expr.idx), eval(expr.val), expr.col);
}
