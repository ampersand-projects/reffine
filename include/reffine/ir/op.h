#ifndef INCLUDE_REFFINE_IR_OP_H_
#define INCLUDE_REFFINE_IR_OP_H_

#include <functional>

#include "reffine/base/type.h"
#include "reffine/ir/node.h"

using namespace std;

namespace reffine {

struct Op : public ExprNode {
    vector<Sym> iters;
    Expr pred;
    vector<Expr> outputs;

    Op(vector<Sym> iters, Expr pred, vector<Expr> outputs)
        : ExprNode(extract_type(iters, outputs)),
          iters(std::move(iters)),
          pred(pred),
          outputs(std::move(outputs))
    {
    }
    Op(std::initializer_list<Sym> iters, Expr pred,
       std::initializer_list<Expr> outputs)
        : Op(vector<Sym>(iters), pred, vector<Expr>(outputs))
    {
    }

    void Accept(Visitor&) final;

private:
    static DataType extract_type(const vector<Sym>& iters,
                                 const vector<Expr>& outputs)
    {
        vector<DataType> dtypes;
        vector<EncodeType> etypes;

        for (const auto& iter : iters) {
            ASSERT(iter->type.is_primitive());
            dtypes.push_back(iter->type);
            etypes.push_back(EncodeType::FLAT);
        }
        for (const auto& output : outputs) {
            ASSERT(output->type.is_val());
            dtypes.push_back(output->type);
            etypes.push_back(EncodeType::FLAT);
        }

        return DataType(BaseType::VECTOR, dtypes, iters.size(), etypes);
    }
};

struct Element : public ExprNode {
    Expr vec;
    Expr iter;

    Element(Expr vec, Expr iter)
        : ExprNode(vec->type.valty()), vec(vec), iter(iter)
    {
        ASSERT(this->vec->type.is_vector());
        ASSERT(this->iter->type == this->vec->type.iterty());
    }

    void Accept(Visitor&) final;
};

struct Lookup : public ExprNode {
    Expr vec;
    Expr idx;

    Lookup(Expr vec, Expr idx) : ExprNode(vec->type.rowty()), vec(vec), idx(idx)
    {
        ASSERT(vec->type.is_vector());
        ASSERT(idx->type.is_idx());
    }

    void Accept(Visitor&) final;
};

struct In : public ExprNode {
    Expr iter;
    Expr vec;

    In(Expr iter, Expr vec) : ExprNode(types::BOOL), iter(iter), vec(vec)
    {
        ASSERT(this->iter->type == this->vec->type.iterty());
    }

    void Accept(Visitor&) final;
};

typedef function<Expr()> InitFnTy;           // () -> state
typedef function<Expr(Expr, Expr)> AccFnTy;  // (state, val) -> state

struct Reduce : public ExprNode {
    Expr vec;
    InitFnTy init;
    AccFnTy acc;

    Reduce(Expr vec, InitFnTy init, AccFnTy acc)
        : ExprNode(init()->type), vec(vec), init(init), acc(acc)
    {
        ASSERT(vec->type.is_vector());
        auto tmp_state = init();
        auto tmp_val = make_shared<SymNode>("tmp_val", vec->type.rowty());
        auto tmp_state2 = acc(tmp_state, tmp_val);

        ASSERT(tmp_state2->type == tmp_state->type);
    }

    void Accept(Visitor&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_OP_H_
