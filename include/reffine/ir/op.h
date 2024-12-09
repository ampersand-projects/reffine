#ifndef INCLUDE_REFFINE_IR_OP_H_
#define INCLUDE_REFFINE_IR_OP_H_

#include <functional>

#include "reffine/base/type.h"
#include "reffine/ir/node.h"

using namespace std;

namespace reffine {

struct Op : public ExprNode {
    vector<Sym> idxs;
    vector<Expr> preds;
    vector<Expr> outputs;

    Op(vector<Sym> idxs, vector<Expr> preds, vector<Expr> outputs)
        : ExprNode(extract_type(idxs, outputs)),
          idxs(std::move(idxs)),
          preds(std::move(preds)),
          outputs(std::move(outputs))
    {
    }

    void Accept(Visitor&) final;

private:
    static DataType extract_type(const vector<Sym>& idxs,
                                 const vector<Expr>& outputs)
    {
        vector<DataType> dtypes;

        for (const auto& idx : idxs) {
            ASSERT(idx->type.is_val());
            dtypes.push_back(idx->type);
        }
        for (const auto& output : outputs) {
            ASSERT(output->type.is_val());
            dtypes.push_back(output->type);
        }

        return DataType(BaseType::VECTOR, std::move(dtypes), idxs.size());
    }
};

struct Element : public ExprNode {
    Expr vec;
    vector<Expr> idxs;

    Element(Expr vec, vector<Expr> idxs)
        : ExprNode(DataType(BaseType::STRUCT, vec->type.dtypes)),
          vec(vec),
          idxs(std::move(idxs))
    {
        const auto& vtype = vec->type;

        // Indexing to a subspace in the vector is not supported yet.
        ASSERT(vtype.dim == idxs.size());

        for (size_t i = 0; i < idxs.size(); i++) {
            ASSERT(vtype.dtypes[i] == idxs[i]->type);
        }
    }

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
        auto tmp_val = make_shared<SymNode>("tmp_val", op.type.valty());
        auto tmp_state2 = acc(tmp_state, tmp_val);

        ASSERT(tmp_state2->type == tmp_state->type);
    }

    void Accept(Visitor&) final;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_OP_H_
