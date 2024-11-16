#ifndef INCLUDE_REFFINE_IR_OP_H_
#define INCLUDE_REFFINE_IR_OP_H_

#include "reffine/base/type.h"
#include "reffine/ir/node.h"

using namespace std;

namespace reffine {

struct Op : public ExprNode {
    vector<Sym> idxs;
    vector<Expr> preds;
    vector<Expr> outputs;

    Op(vector<Sym> idxs, vector<Expr> preds, vector<Expr> outputs) :
        ExprNode(extract_type(idxs, outputs)),
        idxs(std::move(idxs)), preds(std::move(preds)), outputs(std::move(outputs))
    {}

    void Accept(Visitor&) override;

private:
    static DataType extract_type(const vector<Sym>& idxs, const vector<Expr>& outputs)
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

    Element(Expr vec, vector<Expr> idxs) :
        ExprNode(extract_type(vec, idxs)), vec(vec), idxs(std::move(idxs))
    {}

    void Accept(Visitor&) override;

private:
    static DataType extract_type(const Expr& vec, const vector<Expr>& idxs)
    {
        const auto& vtype = vec->type;

        // Indexing to a subspace in the vector is not supported yet.
        ASSERT(vtype.dim == idxs.size());

        for (int i=0; i<idxs.size(); i++) {
            ASSERT(vtype.dtypes[i] == idxs[i]->type);
        }

        return vec->type.valty();
    }
};

typedef function<Expr()> InitFnTy;  // () -> state
typedef function<Expr(Expr, Expr)> AccFnTy;  // (state, val) -> state

struct Reduce : public ExprNode {
    Expr vec;
    InitFnTy init_fn;
    AccFnTy acc_fn;

    Reduce(Expr vec, InitFnTy init_fn, AccFnTy acc_fn) :
        ExprNode(init_fn()->type), vec(vec), init_fn(init_fn), acc_fn(acc_fn)
    {}

    void Accept(Visitor&) override;
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_OP_H_
