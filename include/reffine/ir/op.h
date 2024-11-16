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
        ExprNode(extract_type(idxs, outputs)), idxs(move(idxs)), preds(move(preds)), outputs(move(output))
    {}

    void Accept(Visitor&) override;

private:
    static DataType extract_type(const vector<Sym>& idxs, const vector<Expr>& output)
    {
        vector<DataType> dtypes;

        for (const auto& idx : idxs) {
            dtypes.push_back(idx.type);
        }
        for (const auto& output : outputs) {
            dtypes.push_back(output.type);
        }

        return DataType(BaseType::VECTOR, move(dtypes), idxs.size());
    }
};

struct Element : public ExprNode {
    Expr vec;
    vector<Expr> idxs;

    Element(Expr vec, vector<Expr> idxs) :
        ExprNode(extract_type(vec, idxs)), vec(vec), idxs(move(idxs))
    {}

    void Accept(Visitor&) override;

private:
    static DataType extract_type(const Expr& vec, const vector<Expr>& idxs)
    {
        const auto& vtype = vec->type;

        // Indexing to a subspace in the vector is not supported yet.
        ASSERT(vtype.dim == idxs.size());

        for (int i=0; i<idxs.size(); i++) {
            ASSERT(vtype.dtypes[i] == idxs[i].type);
        }

        return DataType(
            BaseType::STRUCT,
            std::vector<DataType>(vtype.dtypes.begin() + vtype.dim, vtype.dtypes.end())
        );
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_OP_H_
