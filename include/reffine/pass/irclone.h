#ifndef INCLUDE_REFFINE_PASS_IRCLONE_H_
#define INCLUDE_REFFINE_PASS_IRCLONE_H_

#include "reffine/pass/base/irgen.h"

namespace reffine {

class IRClone : public IRGen {
public:
    explicit IRClone(unique_ptr<IRGenCtx> ctx = nullptr) : IRGen(std::move(ctx))
    {
    }

protected:
    Expr visit(Sym) override;
    Expr visit(Select&) override;
    Expr visit(IfElse&) override;
    Expr visit(Const&) override;
    Expr visit(Cast&) override;
    Expr visit(Get&) override;
    Expr visit(New&) override;
    Expr visit(NaryExpr&) override;
    Expr visit(Op&) override;
    Expr visit(Element&) override;
    Expr visit(Lookup&) override;
    Expr visit(In&) override;
    Expr visit(Reduce&) override;
    Expr visit(Call&) override;
    Expr visit(Stmts&) override;
    Expr visit(Alloc&) override;
    Expr visit(Load&) override;
    Expr visit(Store&) override;
    Expr visit(AtomicOp&) override;
    Expr visit(StructGEP&) override;
    Expr visit(ThreadIdx&) override;
    Expr visit(BlockIdx&) override;
    Expr visit(BlockDim&) override;
    Expr visit(GridDim&) override;
    Expr visit(Loop&) override;
    Expr visit(FetchDataPtr&) override;
    Expr visit(NoOp&) override;
    Expr visit(Define&) override;
    Expr visit(InitVal&) override;
    Expr visit(ReadData&) override;
    Expr visit(WriteData&) override;
    Expr visit(ReadBit&) override;
    Expr visit(WriteBit&) override;
    Expr visit(Length&) override;
    Expr visit(SubVector&) override;
    Expr visit(Func&) override;

private:
    shared_ptr<Op> visit_op(Op&);
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_PASS_IRCLONE_H_
