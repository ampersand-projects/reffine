#ifndef INCLUDE_REFFINE_PASS_CEMITTER_H_
#define INCLUDE_REFFINE_PASS_CEMITTER_H_

#include "reffine/pass/iremitter.h"

using namespace std;

namespace reffine {

class CEmitter : public IREmitter {
public:
    CEmitter(unique_ptr<IREmitterCtx> ctx) : IREmitter(std::move(ctx)) {}

    static string Build(Stmt);

    CodeSeg visit(Sym) final;
    CodeSeg visit(Call&) final;
    CodeSeg visit(IfElse&) final;
    CodeSeg visit(NoOp&) final;
    CodeSeg visit(Select&) final;
    CodeSeg visit(Const&) final;
    CodeSeg visit(Get&) final;
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
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_CEMITTER_H_
