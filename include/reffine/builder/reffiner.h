#ifndef INCLUDE_REFFINE_BUILDER_REFFINER_H_
#define INCLUDE_REFFINE_BUILDER_REFFINER_H_

#include "reffine/ir/expr.h"
#include "reffine/ir/loop.h"
#include "reffine/ir/op.h"
#include "reffine/ir/op_to_loop.h"
#include "reffine/ir/stmt.h"

namespace reffine::reffiner {

template <typename T>
struct _stmt : public shared_ptr<T> {
    explicit _stmt(shared_ptr<T>&& ptr) : shared_ptr<T>(std::move(ptr)) {}
};

template <typename T>
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
_expr<Element> _expr_elem(Expr, vector<Expr>);
_expr<NotNull> _expr_notnull(Expr);

template <typename T>
struct _expr : public shared_ptr<T> {
    explicit _expr(shared_ptr<T>&& ptr) : shared_ptr<T>(std::move(ptr)) {}

    _expr<Add> operator+(Expr o) const { return _expr_add(*this, o); }
    _expr<Sub> operator-(Expr o) const { return _expr_sub(*this, o); }
    _expr<Mul> operator*(Expr o) const { return _expr_mul(*this, o); }
    _expr<Div> operator/(Expr o) const { return _expr_div(*this, o); }
    _expr<Neg> operator-() const { return _expr_neg(*this); }
    _expr<Mod> operator%(Expr o) const { return _expr_mod(*this, o); }
    _expr<LessThan> operator<(Expr o) const { return _expr_lt(*this, o); }
    _expr<LessThanEqual> operator<=(Expr o) const
    {
        return _expr_lte(*this, o);
    }
    _expr<GreaterThan> operator>(Expr o) const { return _expr_gt(*this, o); }
    _expr<GreaterThanEqual> operator>=(Expr o) const
    {
        return _expr_gte(*this, o);
    }
    _expr<Equals> operator==(Expr o) const { return _expr_eq(*this, o); }
    _expr<Not> operator!() const { return _expr_not(*this); }
    _expr<And> operator&(Expr o) const { return _expr_and(*this, o); }
    _expr<Or> operator|(Expr o) const { return _expr_or(*this, o); }
    _expr<Get> operator[](size_t n) const { return _expr_get(*this, n); }
    _expr<Element> operator[](std::initializer_list<Expr> iters) const
    {
        return _expr_elem(*this, iters);
    }
    _expr<NotNull> operator~() const { return _expr_notnull(*this); }
};

#define REGISTER_EXPR(NAME, EXPR)                                            \
    template <typename... Args>                                              \
    struct NAME : public _expr<EXPR> {                                       \
        explicit NAME(Args... args)                                          \
            : _expr<EXPR>(                                                   \
                  std::move(make_shared<EXPR>(std::forward<Args>(args)...))) \
        {                                                                    \
        }                                                                    \
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
REGISTER_EXPR(_forall, ForAll)
REGISTER_EXPR(_implies, Implies)
REGISTER_EXPR(_exists, Exists)

// Constant expressions
REGISTER_EXPR(_const, Const)

// Loop expressions
REGISTER_EXPR(_isval, IsValid)
REGISTER_EXPR(_setval, SetValid)
REGISTER_EXPR(_fetch, FetchDataPtr)
REGISTER_EXPR(_fetch_buf, FetchBuffer)
REGISTER_EXPR(_alloc, Alloc)
REGISTER_EXPR(_load, Load)
REGISTER_EXPR(_loop, reffine::Loop)

// Ops
REGISTER_EXPR(_elem, Element)
REGISTER_EXPR(_op, Op)
REGISTER_EXPR(_red, Reduce)
REGISTER_EXPR(_notnull, NotNull)

// Op to Loop
REGISTER_EXPR(_lookup, Lookup)
REGISTER_EXPR(_locate, Locate)
REGISTER_EXPR(_len, Length)

// CUDA
REGISTER_EXPR(_tidx, ThreadIdx)
REGISTER_EXPR(_bidx, BlockIdx)
REGISTER_EXPR(_gdim, GridDim)
REGISTER_EXPR(_bdim, BlockDim)

// Misc expressions
REGISTER_EXPR(_call, Call)
REGISTER_EXPR(_sel, Select)
REGISTER_EXPR(_cast, Cast)
REGISTER_EXPR(_get, Get)
REGISTER_EXPR(_new, New)
REGISTER_EXPR(_nary, NaryExpr)
REGISTER_EXPR(_unary, UnaryExpr)
REGISTER_EXPR(_binary, BinaryExpr)
REGISTER_EXPR(_sym, SymNode)
REGISTER_EXPR(_stmtexpr, StmtExprNode)
REGISTER_EXPR(_structgep, StructGEP)
#undef REGISTER_EXPR

#define REGISTER_STMT(NAME, STMT)                                            \
    template <typename... Args>                                              \
    struct NAME : public _stmt<STMT> {                                       \
        explicit NAME(Args... args)                                          \
            : _stmt<STMT>(                                                   \
                  std::move(make_shared<STMT>(std::forward<Args>(args)...))) \
        {                                                                    \
        }                                                                    \
    };

// Statements
REGISTER_STMT(_func, Func)
REGISTER_STMT(_stmts, Stmts)
REGISTER_STMT(_ifelse, IfElse)
REGISTER_STMT(_noop, NoOp)
REGISTER_STMT(_store, Store)
REGISTER_STMT(_atomic_op, AtomicOp)
#undef REGISTER_STMT

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

const DataType _i8_t = types::INT8;
const DataType _i16_t = types::INT16;
const DataType _i32_t = types::INT32;
const DataType _i64_t = types::INT64;
const DataType _u8_t = types::UINT8;
const DataType _u16_t = types::UINT16;
const DataType _u32_t = types::UINT32;
const DataType _u64_t = types::UINT64;
const DataType _f32_t = types::FLOAT32;
const DataType _f64_t = types::FLOAT64;
const DataType _ch_t = types::INT8;
const DataType _idx_t = types::IDX;
const DataType _bool_t = types::BOOL;

template <size_t dim, typename... Ts>
DataType _vec_t()
{
    return types::VEC<dim, Ts...>();
}

template <typename... Ts>
DataType _struct_t()
{
    return types::STRUCT<Ts...>();
}

}  // namespace reffine::reffiner

#endif  // INCLUDE_REFFINE_BUILDER_REFFINER_H_
