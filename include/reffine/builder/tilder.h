#ifndef INCLUDE_REFFINE_BUILDER_TILDER_H_
#define INCLUDE_REFFINE_BUILDER_TILDER_H_

#include <memory>
#include <utility>
#include <string>

#include "reffine/ir/expr.h"
#include "reffine/ir/stmt.h"
#include "reffine/ir/op.h"
#include "reffine/ir/loop.h"

namespace reffine::tilder {

template<typename T>
struct _expr;

_expr<Add> _expr_add(Expr, Expr);
_expr<Sub> _expr_sub(Expr, Expr);
_expr<Mul> _expr_mul(Expr, Expr);
_expr<Div> _expr_div(Expr, Expr);
_expr<Neg> _expr_neg(Expr);
_expr<Mod> _expr_mod(Expr, Expr);
_expr<LessThan> _expr_lt(Expr, Expr);
_expr<LessThanEqual> _expr_lte(Expr, Expr);
_expr<GreaterThan> _expr_gt(Expr, Expr);
_expr<GreaterThanEqual> _expr_gte(Expr, Expr);
_expr<Equals> _expr_eq(Expr, Expr);
_expr<Not> _expr_not(Expr);
_expr<And> _expr_and(Expr, Expr);
_expr<Or> _expr_or(Expr, Expr);
_expr<Get> _expr_get(Expr, size_t);

template<typename T>
struct _expr : public shared_ptr<T> {
    explicit _expr(shared_ptr<T>&& ptr) : shared_ptr<T>(std::move(ptr)) {}

    _expr<Add> operator+(Expr o) const { return _expr_add(*this, o); }
    _expr<Sub> operator-(Expr o) const { return _expr_sub(*this, o); }
    _expr<Mul> operator*(Expr o) const { return _expr_mul(*this, o); }
    _expr<Div> operator/(Expr o) const { return _expr_div(*this, o); }
    _expr<Neg> operator-() const { return _expr_neg(*this); }
    _expr<Mod> operator%(Expr o) const { return _expr_mod(*this, o); }
    _expr<LessThan> operator<(Expr o) const { return _expr_lt(*this, o); }
    _expr<LessThanEqual> operator<=(Expr o) const { return _expr_lte(*this, o); }
    _expr<GreaterThan> operator>(Expr o) const { return _expr_gt(*this, o); }
    _expr<GreaterThanEqual> operator>=(Expr o) const { return _expr_gte(*this, o); }
    _expr<Equals> operator==(Expr o) const { return _expr_eq(*this, o); }
    _expr<Not> operator!() const { return _expr_not(*this); }
    _expr<And> operator&&(Expr o) const { return _expr_and(*this, o); }
    _expr<Or> operator||(Expr o) const { return _expr_or(*this, o); }
    _expr<Get> operator<<(size_t n) const { return _expr_get(*this, n); }
};

#define REGISTER_EXPR(NAME, EXPR) \
    template<typename... Args> \
    struct NAME : public _expr<EXPR> { \
        explicit NAME(Args... args) : \
            _expr<EXPR>(std::move(make_shared<EXPR>(std::forward<Args>(args)...))) \
        {} \
    };

// Arithmetic expressions
REGISTER_EXPR(_add, Add)
REGISTER_EXPR(_sub, Sub)
REGISTER_EXPR(_mul, Mul)
REGISTER_EXPR(_div, Div)
REGISTER_EXPR(_max, Max)
REGISTER_EXPR(_min, Min)
REGISTER_EXPR(_abs, Abs)
REGISTER_EXPR(_neg, Neg)
REGISTER_EXPR(_mod, Mod)
REGISTER_EXPR(_sqrt, Sqrt)
REGISTER_EXPR(_pow, Pow)
REGISTER_EXPR(_ceil, Ceil)
REGISTER_EXPR(_floor, Floor)
REGISTER_EXPR(_lt, LessThan)
REGISTER_EXPR(_lte, LessThanEqual)
REGISTER_EXPR(_gt, GreaterThan)
REGISTER_EXPR(_gte, GreaterThanEqual)
REGISTER_EXPR(_eq, Equals)

// Logical expressions
REGISTER_EXPR(_not, Not)
REGISTER_EXPR(_and, And)
REGISTER_EXPR(_or, Or)

// Constant expressions
REGISTER_EXPR(_const, Const)

// Loop expressions
REGISTER_EXPR(_isval, IsValid)
REGISTER_EXPR(_setval, SetValid)
REGISTER_EXPR(_fetchptr, FetchDataPtr)
REGISTER_EXPR(_alloc, Alloc)
REGISTER_EXPR(_load, Load)
REGISTER_EXPR(_store, Store)
REGISTER_EXPR(_loop, Loop)

// Statements
REGISTER_EXPR(_func, Func)
REGISTER_EXPR(_stmts, Stmt)
REGISTER_EXPR(_ifelse, IfElse)
REGISTER_EXPR(_noop, NoOp)

// Ops
REGISTER_EXPR(_elem, Element)
REGISTER_EXPR(_op, Op)
REGISTER_EXPR(_red, Reduce)

// Misc expressions
REGISTER_EXPR(_call, Call)
REGISTER_EXPR(_sel, Select)
REGISTER_EXPR(_cast, Cast)
REGISTER_EXPR(_get, Get)
REGISTER_EXPR(_new, New)
REGISTER_EXPR(_nary, NaryExpr)
REGISTER_EXPR(_unary, UnaryExpr)
REGISTER_EXPR(_binary, BinaryExpr)

#undef REGISTER_EXPR


_expr<Const> _i8(int8_t);
_expr<Const> _i16(int16_t);
_expr<Const> _i32(int32_t);
_expr<Const> _i64(int64_t);
_expr<Const> _u8(uint8_t);
_expr<Const> _u16(uint16_t);
_expr<Const> _u32(uint32_t);
_expr<Const> _u64(uint64_t);
_expr<Const> _f32(float);
_expr<Const> _f64(double);
_expr<Const> _ch(char);
_expr<Const> _idx(int64_t);
_expr<Const> _true();
_expr<Const> _false();

}  // namespace reffine::tilder

#endif  // INCLUDE_REFFINE_BUILDER_TILDER_H_