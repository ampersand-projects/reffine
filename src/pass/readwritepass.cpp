#include "reffine/pass/readwritepass.h"

#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;

Expr ReadWritePass::visit(ReadRunEnd& expr)
{
    auto buf = _cast(types::INT32.ptr(),
                     _arrbuf(_arrchild(_arrchild(_vecarr(eval(expr.vec)), expr.col), 0), 1));
    return _cast(types::IDX, _load(buf, eval(expr.idx)));
}

Expr ReadWritePass::visit(ReadData& expr)
{
    Expr buf;
    if (expr.vec->type.encodings[expr.col] == EncodeType::RUNEND) {
        buf = _cast(expr.type.ptr(),
                _arrbuf(_arrchild(_arrchild(_vecarr(eval(expr.vec)), expr.col), 1), 1));
    } else {
        buf = _cast(expr.type.ptr(),
                _arrbuf(_arrchild(_vecarr(eval(expr.vec)), expr.col), 1));
    }

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
    if (expr.vec->type.encodings[expr.col] == EncodeType::RUNEND) {
        throw runtime_error("RunEnd not supported for ReadBit");
    } else {
        return _isval(eval(expr.vec), eval(expr.idx), expr.col);
    }
}

Expr ReadWritePass::visit(WriteBit& expr)
{
    if (expr.vec->type.encodings[expr.col] == EncodeType::RUNEND) {
        throw runtime_error("RunEnd not supported for WriteBit");
    } else {
        return _setval(eval(expr.vec), eval(expr.idx), eval(expr.val), expr.col);
    }
}

Expr ReadWritePass::visit(Length& expr)
{
    if (expr.col < expr.vec->type.dim - 1) {
        return _arrlen(_arrchild(_arrchild(_vecarr(eval(expr.vec)), expr.col), 1));
    } else {
        return _arrlen(_arrchild(_vecarr(eval(expr.vec)), expr.col));
    }
}
