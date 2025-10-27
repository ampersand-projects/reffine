#include "reffine/pass/readwritepass.h"

#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

Expr ReadWritePass::visit(ReadData& expr)
{
    auto buf = _cast(expr.type.ptr(),
                     _arrbuf(_arrchild(_vecarr(eval(expr.vec)), expr.col), 1));
    return _load(buf, eval(expr.idx));
}

Expr ReadWritePass::visit(WriteData& expr)
{
    auto buf = _cast(expr.val->type.ptr(),
                     _arrbuf(_arrchild(_vecarr(eval(expr.vec)), expr.col), 1));
    return _store(buf, eval(expr.val), eval(expr.idx));
}

Expr ReadWritePass::visit(ReadBit& expr)
{
    return _isval(eval(expr.vec), eval(expr.idx), expr.col);
}

Expr ReadWritePass::visit(WriteBit& expr)
{
    return _setval(eval(expr.vec), eval(expr.idx), eval(expr.val), expr.col);
}

Expr ReadWritePass::visit(Length& expr)
{
    return _arrlen(_arrchild(_vecarr(eval(expr.vec)), expr.col));
}
