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

        for (const auto& iter : iters) {
            ASSERT(iter->type.is_val());
            dtypes.push_back(iter->type);
        }
        for (const auto& output : outputs) {
            ASSERT(output->type.is_val());
            dtypes.push_back(output->type);
        }

        return DataType(BaseType::VECTOR, std::move(dtypes), iters.size());
    }
};

struct Element : public ExprNode {
    Expr vec;
    vector<Expr> iters;

    Element(Expr vec, vector<Expr> iters)
        : ExprNode(vec->type.elemty(iters.size())),
          vec(vec),
          iters(std::move(iters))
    {
        const auto& vtype = vec->type;

        for (const auto& iter : iters) { ASSERT(iter->type.is_val()); }
        ASSERT(vtype.is_vector());

        ASSERT(vtype.dim == this->iters.size());
        for (size_t i = 0; i < this->iters.size(); i++) {
            ASSERT(vtype.dtypes[i] == this->iters[i]->type);
        }
    }
    Element(Expr vec, std::initializer_list<Expr> iters)
        : Element(vec, vector<Expr>(iters))
    {
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

struct NotNull : public ExprNode {
    Expr elem;

    NotNull(Expr elem) : ExprNode(types::BOOL), elem(elem) {}

    void Accept(Visitor&) final;
};

typedef function<Expr()> InitFnTy;           // () -> state
typedef function<Expr(Expr, Expr)> AccFnTy;  // (state, val) -> state

struct Reduce : public ExprNode {
    Op op;
    InitFnTy init;
    AccFnTy acc;

    Reduce(Op op, InitFnTy init, AccFnTy acc)
        : ExprNode(init()->type), op(std::move(op)), init(init), acc(acc)
    {
        auto tmp_state = init();
        auto tmp_val = make_shared<SymNode>("tmp_val", op.type.rowty());
        auto tmp_state2 = acc(tmp_state, tmp_val);

        ASSERT(tmp_state2->type == tmp_state->type);
    }

    void Accept(Visitor&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_OP_H_
