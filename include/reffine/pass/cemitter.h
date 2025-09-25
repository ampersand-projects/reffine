#ifndef INCLUDE_REFFINE_PASS_CEMITTER_H_
#define INCLUDE_REFFINE_PASS_CEMITTER_H_

#include <unordered_map>

#include "reffine/pass/iremitter.h"

using namespace std;

namespace reffine {

class CEmitter : public IREmitter {
public:
    CEmitter(unique_ptr<IREmitterCtx> ctx)
        : IREmitter(std::move(ctx)), _header(make_shared<BlockSeg>())
    {
        this->_header->emit("#include <algorithm>", nl());
        this->_header->emit("#include \"vinstr/internal.cpp\"", nl());
        this->_header->emit(nl());
    }

    static string Build(Expr);

    CodeSeg visit(Sym) final;
    CodeSeg visit(Call&) final;
    CodeSeg visit(IfElse&) final;
    CodeSeg visit(NoOp&) final;
    CodeSeg visit(Select&) final;
    CodeSeg visit(Const&) final;
    CodeSeg visit(Cast&) final;
    CodeSeg visit(NaryExpr&) final;
    CodeSeg visit(Stmts&) final;
    CodeSeg visit(Alloc&) final;
    CodeSeg visit(Load&) final;
    CodeSeg visit(Store&) final;
    CodeSeg visit(Loop&) final;
    CodeSeg visit(StructGEP&) final;
    CodeSeg visit(FetchDataPtr&) final;
    CodeSeg visit(Func&) final;

private:
    string get_type_str(DataType);

    shared_ptr<MultiSeg> _header;
    unordered_map<DataType, string> _type_name_map;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_CEMITTER_H_
