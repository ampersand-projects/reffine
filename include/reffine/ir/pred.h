#ifndef INCLUDE_REFFINE_IR_PRED_H_
#define INCLUDE_REFFINE_IR_PRED_H_

#include "reffine/ir/expr.h"

using namespace std;

namespace reffine {

struct Implies : public BinaryExpr {
    Implies(Expr a, Expr b) : BinaryExpr(types::BOOL, MathOp::IMPLIES, a, b)
    {
        ASSERT(a->type == types::BOOL);
        ASSERT(b->type == types::BOOL);
    }
};

struct Iff : public BinaryExpr {
    Iff(Expr a, Expr b) : BinaryExpr(types::BOOL, MathOp::IFF, a, b)
    {
        ASSERT(a->type == types::BOOL);
        ASSERT(b->type == types::BOOL);
    }
};

struct ForAll : public NaryExpr {
    ForAll(vector<Expr> iters, Expr b)
        : NaryExpr(types::BOOL, MathOp::FORALL, iters)
    {
        ASSERT(iters.size() > 0);
        this->args.push_back(b);
        ASSERT(b->type == types::BOOL);
    }
    ForAll(Sym iter, Expr b) : ForAll(vector<Expr>{iter}, b) {}
};

struct Exists : public NaryExpr {
    Exists(Sym a, Expr b)
        : NaryExpr(types::BOOL, MathOp::EXISTS, vector<Expr>{a, b})
    {
        ASSERT(b->type == types::BOOL);
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_PRED_H_
