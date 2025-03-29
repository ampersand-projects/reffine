#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "reffine/ir/expr.h"
#include "reffine/ir/loop.h"
#include "reffine/ir/op.h"
#include "reffine/ir/stmt.h"
#include "reffine/base/type.h"
#include "reffine/builder/reffiner.h"
#include "reffine/pass/printer.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

namespace py = pybind11;

void print_IR(shared_ptr<Func> fn)
 {
     cout << "Loop IR:" << endl << IRPrinter::Build(fn) << endl;
 }

#define REGISTER_CLASS(CLASS, PARENT, MODULE, NAME, ...) \
    py::class_<CLASS, shared_ptr<CLASS>, PARENT>(MODULE, NAME) \
        .def(py::init<__VA_ARGS__>());

PYBIND11_MODULE(ir, m) {
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
        .value("struct", BaseType::STRUCT);

    py::class_<DataType>(m, "DataType")
        .def(py::init<BaseType, vector<DataType>, size_t>(),
              py::arg("btype"),
              py::arg("dtypes") = vector<DataType>{},
              py::arg("size") = 0)
        .def("str", &DataType::str);

    /* StmtNode and Derived Structures Declarations
    */
    py::class_<StmtNode, Stmt>(m, "stmt");
    py::class_<ExprNode, Expr, StmtNode>(m, "expr");
    
    /* Symbol Definition */
    py::class_<SymNode, Sym, ExprNode>(m, "sym")
        .def(py::init<string, DataType>())
        .def(py::init<string, Expr>());

    /* Constant Expressions */
    REGISTER_CLASS(Const, ExprNode, m, "const", DataType, double)

    /* Loop ops */
    REGISTER_CLASS(IsValid, ExprNode, m, "is_valid", Expr, Expr, size_t)
    REGISTER_CLASS(SetValid, ExprNode, m, "set_valid", Expr, Expr, Expr, size_t)
    REGISTER_CLASS(FetchDataPtr, ExprNode, m, "fetch", Expr, Expr, size_t)
    REGISTER_CLASS(Alloc, ExprNode, m, "alloc", DataType, Expr)
    REGISTER_CLASS(Load, ExprNode, m, "load", Expr)
    REGISTER_CLASS(Store, StmtNode, m, "store", Expr, Expr)
    REGISTER_CLASS(Loop, ExprNode, m, "loop", Expr)

    /* Statements */
    REGISTER_CLASS(Func, StmtNode, m, "func", string, Expr, vector<Sym>, SymTable)
    REGISTER_CLASS(Stmts, StmtNode, m, "stmts", vector<Stmt>)
    REGISTER_CLASS(IfElse, StmtNode, m, "ifelse", Expr, Stmt, Stmt)
    REGISTER_CLASS(NoOp, StmtNode, m, "noop")

    /* Math Operators for Nary Expressions */
    py::enum_<MathOp>(m, "MathOp")
        .value("add", MathOp::ADD)
        .value("sub", MathOp::SUB)
        .value("mul", MathOp::MUL)
        .value("div", MathOp::DIV)
        .value("max", MathOp::MAX)
        .value("min", MathOp::MIN)
        .value("mod", MathOp::MOD)
        .value("sqrt", MathOp::SQRT)
        .value("pow", MathOp::POW)
        .value("abs", MathOp::ABS)
        .value("neg", MathOp::NEG)
        .value("ceil", MathOp::CEIL)
        .value("floor", MathOp::FLOOR)
        .value("lt", MathOp::LT)
        .value("lte", MathOp::LTE)
        .value("gt", MathOp::GT)
        .value("gte", MathOp::GTE)
        .value("eq", MathOp::EQ)
        .value("not", MathOp::NOT)
        .value("and", MathOp::AND)
        .value("or", MathOp::OR);

    /* Nary Expressions */
    REGISTER_CLASS(NaryExpr, ExprNode, m, "nary_expr", DataType, MathOp, vector<Expr>)
    REGISTER_CLASS(UnaryExpr, NaryExpr, m, "unary_expr", DataType, MathOp, Expr)
    REGISTER_CLASS(BinaryExpr, NaryExpr, m, "binary_expr", DataType, MathOp, Expr, Expr)

    /* Logical Expressions */
    REGISTER_CLASS(Equals, BinaryExpr, m, "equals", Expr, Expr)
    REGISTER_CLASS(And, BinaryExpr, m, "and", Expr, Expr)
    REGISTER_CLASS(Or, BinaryExpr, m, "or", Expr, Expr)
    REGISTER_CLASS(Implies, BinaryExpr, m, "implies", Expr, Expr)
    REGISTER_CLASS(ForAll, NaryExpr, m, "for_all", Sym, Expr)
    REGISTER_CLASS(Exists, NaryExpr, m, "exists", Sym, Expr)

    /* Comparison Operators*/
    REGISTER_CLASS(LessThan, BinaryExpr, m, "lt", Expr, Expr)
    REGISTER_CLASS(GreaterThan, BinaryExpr, m, "gt", Expr, Expr)
    REGISTER_CLASS(LessThanEqual, BinaryExpr, m, "lte", Expr, Expr)
    REGISTER_CLASS(GreaterThanEqual, BinaryExpr, m, "gte", Expr, Expr)

    /* Misc Expressions */
    REGISTER_CLASS(Get, ExprNode, m, "get", Expr, size_t)
    REGISTER_CLASS(New, ExprNode, m, "new", vector<Expr>)
    REGISTER_CLASS(Cast, ExprNode, m, "cast", DataType, Expr)

    m.def("print_IR", &print_IR);
}
