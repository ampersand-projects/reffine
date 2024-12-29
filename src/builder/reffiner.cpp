#include "reffine/builder/reffiner.h"

#include "reffine/base/type.h"

namespace reffine::reffiner {

_expr<Add> _expr_add(Expr a, Expr b) { return _add(a, b); }
_expr<Sub> _expr_sub(Expr a, Expr b) { return _sub(a, b); }
_expr<Mul> _expr_mul(Expr a, Expr b) { return _mul(a, b); }
_expr<Div> _expr_div(Expr a, Expr b) { return _div(a, b); }
_expr<Neg> _expr_neg(Expr a) { return _neg(a); }
_expr<Mod> _expr_mod(Expr a, Expr b) { return _mod(a, b); }
_expr<LessThan> _expr_lt(Expr a, Expr b) { return _lt(a, b); }
_expr<LessThanEqual> _expr_lte(Expr a, Expr b) { return _lte(a, b); }
_expr<GreaterThan> _expr_gt(Expr a, Expr b) { return _gt(a, b); }
_expr<GreaterThanEqual> _expr_gte(Expr a, Expr b) { return _gte(a, b); }
_expr<Equals> _expr_eq(Expr a, Expr b) { return _eq(a, b); }
_expr<Not> _expr_not(Expr a) { return _not(a); }
_expr<And> _expr_and(Expr a, Expr b) { return _and(a, b); }
_expr<Or> _expr_or(Expr a, Expr b) { return _or(a, b); }
_expr<Get> _expr_get(Expr a, size_t n) { return _get(a, n); }

_expr<Const> _i8(int8_t v) { return _const(types::INT8, v); }
_expr<Const> _i16(int16_t v) { return _const(types::INT16, v); }
_expr<Const> _i32(int32_t v) { return _const(types::INT32, v); }
_expr<Const> _i64(int64_t v) { return _const(types::INT64, v); }
_expr<Const> _u8(uint8_t v) { return _const(types::UINT8, v); }
_expr<Const> _u16(uint16_t v) { return _const(types::UINT16, v); }
_expr<Const> _u32(uint32_t v) { return _const(types::UINT32, v); }
_expr<Const> _u64(uint64_t v) { return _const(types::UINT64, v); }
_expr<Const> _f32(float v) { return _const(types::FLOAT32, v); }
_expr<Const> _f64(double v) { return _const(types::FLOAT64, v); }
_expr<Const> _idx(int64_t v) { return _const(types::IDX, v); }
_expr<Const> _true() { return _const(types::BOOL, 1); }
_expr<Const> _false() { return _const(types::BOOL, 0); }

}  // namespace reffine::reffiner
