#ifndef INCLUDE_REFFINE_IR_EXPR_H_
#define INCLUDE_REFFINE_IR_EXPR_H_

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <utility>

#include "reffine/base/type.h"
#include "reffine/base/log.h"
#include "reffine/ir/node.h"

using namespace std;

namespace reffine {

struct Func : public StmtNode {
    string name;
    Expr output;
    vector<Sym> inputs;
    SymTable tbl;

    Func(string name, Expr output, vector<Sym> inputs, SymTable tbl = {}) :
        StmtNode(), name(name), output(output), inputs(std::move(inputs)), tbl(std::move(tbl))
    {}

    void Accept(Visitor&) const override;
};

struct Call : public ExprNode {
    string name;
    vector<Expr> args;

    Call(string name, DataType type, vector<Expr> args) :
        ExprNode(type), name(name), args(std::move(args))
    {}

    void Accept(Visitor&) const final;
};

struct IfElse : public ExprNode {
    Expr cond;
    Expr true_body;
    Expr false_body;

    IfElse(Expr cond, Expr true_body, Expr false_body) :
        ExprNode(true_body->type), cond(cond), true_body(true_body), false_body(false_body)
    {
        ASSERT(cond->type == types::BOOL);
        ASSERT(true_body->type == false_body->type);
    }

    void Accept(Visitor&) const final;
};


struct Select : public ExprNode {
    Expr cond;
    Expr true_body;
    Expr false_body;

    Select(Expr cond, Expr true_body, Expr false_body) :
        ExprNode(true_body->type), cond(cond), true_body(true_body), false_body(false_body)
    {
        ASSERT(cond->type == types::BOOL);
        ASSERT(true_body->type == false_body->type);
    }

    void Accept(Visitor&) const final;
};

struct Const : public ExprNode {
    const double val;

    Const(BaseType btype, double val) :
        ExprNode(DataType(btype)), val(val)
    {}

    void Accept(Visitor&) const final;
};

struct Exists : public ExprNode {
    Sym sym;

    explicit Exists(Sym sym) : ExprNode(types::BOOL), sym(sym) {}

    void Accept(Visitor&) const final;
};

struct Cast : public ExprNode {
    Expr arg;

    Cast(DataType type, Expr arg) : ExprNode(type), arg(arg)
    {
        ASSERT(!arg->type.is_struct() && !type.is_struct());
    }

    void Accept(Visitor&) const final;
};

struct NaryExpr : public ExprNode {
    MathOp op;
    vector<Expr> args;

    NaryExpr(DataType type, MathOp op, vector<Expr> args) :
        ExprNode(type), op(op), args(std::move(args))
    {
        ASSERT(!arg(0)->type.is_ptr() && !arg(0)->type.is_struct());
    }

    Expr arg(size_t i) const { return args[i]; }

    size_t size() const { return args.size(); }

    void Accept(Visitor&) const final;
};

struct UnaryExpr : public NaryExpr {
    UnaryExpr(DataType type, MathOp op, Expr input)
        : NaryExpr(type, op, vector<Expr>{input})
    {}
};

struct BinaryExpr : public NaryExpr {
    BinaryExpr(DataType type, MathOp op, Expr left, Expr right)
        : NaryExpr(type, op, vector<Expr>{left, right})
    {
        ASSERT(left->type == right->type);
    }
};

struct Not : public UnaryExpr {
    explicit Not(Expr a) : UnaryExpr(types::BOOL, MathOp::NOT, a)
    {
        ASSERT(a->type == types::BOOL);
    }
};

struct Abs : public UnaryExpr {
    explicit Abs(Expr a) : UnaryExpr(a->type, MathOp::ABS, a) {}
};

struct Neg : public UnaryExpr {
    explicit Neg(Expr a) : UnaryExpr(a->type, MathOp::NEG, a) {}
};

struct Sqrt : public UnaryExpr {
    explicit Sqrt(Expr a) : UnaryExpr(a->type, MathOp::SQRT, a) {}
};

struct Ceil : public UnaryExpr {
    explicit Ceil(Expr a) : UnaryExpr(a->type, MathOp::CEIL, a) {
        ASSERT(a->type.is_float());
    }
};

struct Floor : public UnaryExpr {
    explicit Floor(Expr a) : UnaryExpr(a->type, MathOp::FLOOR, a) {
        ASSERT(a->type.is_float());
    }
};

struct Equals : public BinaryExpr {
    Equals(Expr a, Expr b) : BinaryExpr(types::BOOL, MathOp::EQ, a, b) {}
};

struct And : public BinaryExpr {
    And(Expr a, Expr b) : BinaryExpr(types::BOOL, MathOp::AND, a, b)
    {
        ASSERT(a->type == types::BOOL);
    }
};

struct Or : public BinaryExpr {
    Or(Expr a, Expr b) : BinaryExpr(types::BOOL, MathOp::OR, a, b)
    {
        ASSERT(a->type == types::BOOL);
    }
};

struct LessThan : public BinaryExpr {
    LessThan(Expr a, Expr b) : BinaryExpr(types::BOOL, MathOp::LT, a, b) {}
};

struct GreaterThan : public BinaryExpr {
    GreaterThan(Expr a, Expr b) : BinaryExpr(types::BOOL, MathOp::GT, a, b) {}
};

struct LessThanEqual : public BinaryExpr {
    LessThanEqual(Expr a, Expr b) : BinaryExpr(types::BOOL, MathOp::LTE, a, b) {}
};

struct GreaterThanEqual : public BinaryExpr {
    GreaterThanEqual(Expr a, Expr b) : BinaryExpr(types::BOOL, MathOp::GTE, a, b) {}
};

struct Add : public BinaryExpr {
    Add(Expr a, Expr b) : BinaryExpr(a->type, MathOp::ADD, a, b) {}
};

struct Sub : public BinaryExpr {
    Sub(Expr a, Expr b) : BinaryExpr(a->type, MathOp::SUB, a, b) {}
};

struct Mul : public BinaryExpr {
    Mul(Expr a, Expr b) : BinaryExpr(a->type, MathOp::MUL, a, b) {}
};

struct Div : public BinaryExpr {
    Div(Expr a, Expr b) : BinaryExpr(a->type, MathOp::DIV, a, b) {}
};

struct Max : public BinaryExpr {
    Max(Expr a, Expr b) : BinaryExpr(a->type, MathOp::MAX, a, b) {}
};

struct Min : public BinaryExpr {
    Min(Expr a, Expr b) : BinaryExpr(a->type, MathOp::MIN, a, b) {}
};

struct Mod : public BinaryExpr {
    Mod(Expr a, Expr b) : BinaryExpr(a->type, MathOp::MOD, a, b)
    {
        ASSERT(!a->type.is_float());
    }
};

struct Pow : public BinaryExpr {
    Pow(Expr a, Expr b) : BinaryExpr(a->type, MathOp::POW, a, b) {
        ASSERT(a->type.is_float());
    }
};

}  // namespace reffine

#endif  // INCLUDE_REFFINE_IR_EXPR_H_
