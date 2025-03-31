#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "reffine/base/type.h"
#include "reffine/builder/reffiner.h"
#include "reffine/ir/expr.h"
#include "reffine/ir/loop.h"
#include "reffine/ir/op.h"
#include "reffine/ir/stmt.h"
#include "reffine/pass/printer.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

namespace py = pybind11;

string to_string(shared_ptr<Func> fn) { return IRPrinter::Build(fn); }

#define REGISTER_CLASS(CLASS, PARENT, MODULE, NAME, ...)       \
    py::class_<CLASS, shared_ptr<CLASS>, PARENT>(MODULE, NAME) \
        .def(py::init<__VA_ARGS__>());

PYBIND11_MODULE(ir, m)
{
    /* Structures related to Reffine typing */
    py::enum_<BaseType>(m, "BaseType")
        .value("bool", BaseType::BOOL)
        .value("i8", BaseType::INT8)
        .value("i16", BaseType::INT16)
        .value("i32", BaseType::INT32)
        .value("i64", BaseType::INT64)
        .value("u8", BaseType::UINT8)
        .value("u16", BaseType::UINT16)
        .value("u32", BaseType::UINT32)
        .value("u64", BaseType::UINT64)
        .value("f32", BaseType::FLOAT32)
        .value("f64", BaseType::FLOAT64)
        .value("struct", BaseType::STRUCT)
        .value("ptr", BaseType::PTR);

    py::class_<DataType>(m, "DataType")
        .def(py::init<BaseType, vector<DataType>, size_t>(), py::arg("btype"),
             py::arg("dtypes") = vector<DataType>{}, py::arg("size") = 0)
        .def("str", &DataType::str);

    /* StmtNode and Derived Structures Declarations
     */
    py::class_<StmtNode, Stmt>(m, "stmt");
    py::class_<ExprNode, Expr, StmtNode>(m, "expr");

    /* Symbol Definition */
    py::class_<SymNode, Sym, ExprNode>(m, "sym")
        .def(py::init<string, DataType>())
        .def(py::init<string, Expr>());

    /* Const */
    REGISTER_CLASS(Const, ExprNode, m, "const", DataType, double)

    /* Loop */
    REGISTER_CLASS(IsValid, ExprNode, m, "_is_valid", Expr, Expr, size_t)
    REGISTER_CLASS(SetValid, ExprNode, m, "_set_valid", Expr, Expr, Expr, size_t)
    REGISTER_CLASS(FetchDataPtr, ExprNode, m, "_fetch", Expr, Expr, size_t)
    REGISTER_CLASS(Alloc, ExprNode, m, "_alloc", DataType, Expr)
    REGISTER_CLASS(Load, ExprNode, m, "_load", Expr)
    REGISTER_CLASS(Store, StmtNode, m, "_store", Expr, Expr)
    REGISTER_CLASS(Loop, ExprNode, m, "_loop", Expr)

    /* Op */
    REGISTER_CLASS(Op, ExprNode, m, "_op", vector<Sym>, Expr, vector<Expr>)

    /* Statements */
    REGISTER_CLASS(Func, StmtNode, m, "_func", string, Expr, vector<Sym>,
                   SymTable)
    REGISTER_CLASS(Stmts, StmtNode, m, "_stmts", vector<Stmt>)
    REGISTER_CLASS(IfElse, StmtNode, m, "_ifelse", Expr, Stmt, Stmt)
    REGISTER_CLASS(NoOp, StmtNode, m, "_noop")

    /* Misc Expressions */
    REGISTER_CLASS(Call, ExprNode, m, "_call", string, DataType, vector<Expr>)
    REGISTER_CLASS(Select, ExprNode, m, "_select", Expr, Expr, Expr)
    REGISTER_CLASS(Cast, ExprNode, m, "_cast", DataType, Expr)
    REGISTER_CLASS(Get, ExprNode, m, "_get", Expr, size_t)
    REGISTER_CLASS(New, ExprNode, m, "_new", vector<Expr>)

    /* Math Operators for Nary Expressions */
    py::enum_<MathOp>(m, "MathOp")
        .value("_add", MathOp::ADD)
        .value("_sub", MathOp::SUB)
        .value("_mul", MathOp::MUL)
        .value("_div", MathOp::DIV)
        .value("_max", MathOp::MAX)
        .value("_min", MathOp::MIN)
        .value("_mod", MathOp::MOD)
        .value("_sqrt", MathOp::SQRT)
        .value("_pow", MathOp::POW)
        .value("_abs", MathOp::ABS)
        .value("_neg", MathOp::NEG)
        .value("_ceil", MathOp::CEIL)
        .value("_floor", MathOp::FLOOR)
        .value("_lt", MathOp::LT)
        .value("_lte", MathOp::LTE)
        .value("_gt", MathOp::GT)
        .value("_gte", MathOp::GTE)
        .value("_eq", MathOp::EQ)
        .value("_not", MathOp::NOT)
        .value("_and", MathOp::AND)
        .value("_or", MathOp::OR);

    /* Nary Expressions */
    REGISTER_CLASS(NaryExpr, ExprNode, m, "_nary_expr", DataType, MathOp,
                   vector<Expr>)
    REGISTER_CLASS(UnaryExpr, NaryExpr, m, "_unary_expr", DataType, MathOp, Expr)
    REGISTER_CLASS(BinaryExpr, NaryExpr, m, "_binary_expr", DataType, MathOp,
                   Expr, Expr)

    /* Math ops */
    REGISTER_CLASS(Not, UnaryExpr, m, "_not", Expr)
    REGISTER_CLASS(Abs, UnaryExpr, m, "_abs", Expr)
    REGISTER_CLASS(Neg, UnaryExpr, m, "_neg", Expr)
    REGISTER_CLASS(Sqrt, UnaryExpr, m, "_sqrt", Expr)
    REGISTER_CLASS(Ceil, UnaryExpr, m, "_ceil", Expr)
    REGISTER_CLASS(Floor, UnaryExpr, m, "_floor", Expr)
    REGISTER_CLASS(Add, BinaryExpr, m, "_add", Expr, Expr)
    REGISTER_CLASS(Sub, BinaryExpr, m, "_sub", Expr, Expr)
    REGISTER_CLASS(Mul, BinaryExpr, m, "_mul", Expr, Expr)
    REGISTER_CLASS(Div, BinaryExpr, m, "_div", Expr, Expr)
    REGISTER_CLASS(Max, BinaryExpr, m, "_max", Expr, Expr)
    REGISTER_CLASS(Min, BinaryExpr, m, "_min", Expr, Expr)
    REGISTER_CLASS(Mod, BinaryExpr, m, "_mod", Expr, Expr)
    REGISTER_CLASS(Pow, BinaryExpr, m, "_pow", Expr, Expr)
    REGISTER_CLASS(Equals, BinaryExpr, m, "_eq", Expr, Expr)
    REGISTER_CLASS(And, BinaryExpr, m, "_and", Expr, Expr)
    REGISTER_CLASS(Or, BinaryExpr, m, "_or", Expr, Expr)

    /* Logical Expressions */
    REGISTER_CLASS(Implies, BinaryExpr, m, "_implies", Expr, Expr)
    REGISTER_CLASS(ForAll, NaryExpr, m, "_for_all", Sym, Expr)
    REGISTER_CLASS(Exists, NaryExpr, m, "_exists", Sym, Expr)

    /* Comparison Operators*/
    REGISTER_CLASS(LessThan, BinaryExpr, m, "_lt", Expr, Expr)
    REGISTER_CLASS(GreaterThan, BinaryExpr, m, "_gt", Expr, Expr)
    REGISTER_CLASS(LessThanEqual, BinaryExpr, m, "_lte", Expr, Expr)
    REGISTER_CLASS(GreaterThanEqual, BinaryExpr, m, "_gte", Expr, Expr)

    m.def("to_string", [](std::shared_ptr<Func> fn) { return to_string(fn); });
}
