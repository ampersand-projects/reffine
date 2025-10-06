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

#define REGISTER_CLASS(CLASS, PARENT, MODULE, NAME, ...)       \
    py::class_<CLASS, shared_ptr<CLASS>, PARENT>(MODULE, NAME) \
        .def(py::init<__VA_ARGS__>());

void init_ir(py::module_& m)
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
        .def("__eq__", &DataType::operator==)
        .def("is_struct", &DataType::is_struct)
        .def("is_ptr", &DataType::is_ptr)
        .def("is_idx", &DataType::is_idx)
        .def("is_vector", &DataType::is_vector)
        .def("is_void", &DataType::is_void)
        .def("is_val", &DataType::is_val)
        .def("is_float", &DataType::is_float)
        .def("is_primitive", &DataType::is_primitive)
        .def("is_int", &DataType::is_int)
        .def("is_signed", &DataType::is_signed)
        .def("ptr", &DataType::ptr)
        .def("deref", &DataType::deref)
        .def("iterty", &DataType::iterty)
        .def("valty", &DataType::valty)
        .def("str", &DataType::str);

    /* StmtNode and Derived Structures Declarations
     */
    py::class_<ExprNode, Expr>(m, "_expr");
    py::class_<StmtNode, Stmt, ExprNode>(m, "_stmt");

    /* Symbol Definition */
    py::class_<SymNode, Sym, ExprNode>(m, "_sym")
        .def(py::init<string, DataType>())
        .def(py::init<string, Expr>());

    /* Const */
    REGISTER_CLASS(Const, ExprNode, m, "_const", DataType, double)

    /* Loop */
    REGISTER_CLASS(IsValid, ExprNode, m, "_isval", Expr, Expr)
    REGISTER_CLASS(SetValid, ExprNode, m, "_setvald", Expr, Expr, Expr)
    REGISTER_CLASS(FetchDataPtr, ExprNode, m, "_fetch", Expr, size_t)
    REGISTER_CLASS(Alloc, ExprNode, m, "_alloc", DataType, Expr)
    REGISTER_CLASS(Load, ExprNode, m, "_load", Expr, Expr)
    py::class_<Loop, shared_ptr<Loop>, ExprNode>(m, "_loop")
        .def(py::init<Expr>())
        .def_readwrite("init", &Loop::init)
        .def_readwrite("body", &Loop::body)
        .def_readwrite("exit_cond", &Loop::exit_cond)
        .def_readwrite("post", &Loop::post);

    /* Op */
    REGISTER_CLASS(Element, ExprNode, m, "_elem", Expr, Expr)
    REGISTER_CLASS(Op, ExprNode, m, "_op", vector<Sym>, Expr, vector<Expr>)
    REGISTER_CLASS(Reduce, ExprNode, m, "_red", Expr, InitFnTy, AccFnTy)
    REGISTER_CLASS(In, ExprNode, m, "_in", Expr, Expr)

    /* Statements */
    REGISTER_CLASS(Stmts, StmtNode, m, "_stmts", vector<Expr>)
    REGISTER_CLASS(IfElse, StmtNode, m, "_ifelse", Expr, Expr, Expr)
    REGISTER_CLASS(NoOp, StmtNode, m, "_noop")
    REGISTER_CLASS(Store, StmtNode, m, "_store", Expr, Expr, Expr)
    py::class_<Func, shared_ptr<Func>, StmtNode>(m, "_func")
        .def(py::init<string, Expr, vector<Sym>, SymTable, bool>(),
             py::arg("name"), py::arg("output"), py::arg("inputs"),
             py::arg("tbl") = SymTable(), py::arg("is_kernel") = false)
        .def_readwrite("name", &Func::name)
        .def_readwrite("output", &Func::output)
        .def_readwrite("inputs", &Func::inputs)
        .def_readwrite("tbl", &Func::tbl)
        .def_readwrite("is_kernel", &Func::is_kernel)
        .def("insert_sym",
             [](Func& f, const Sym& sym, Expr& expr) { f.tbl[sym] = expr; });

    /* Misc Expressions */
    REGISTER_CLASS(Call, ExprNode, m, "_call", string, DataType, vector<Expr>)
    REGISTER_CLASS(Select, ExprNode, m, "_select", Expr, Expr, Expr)
    REGISTER_CLASS(Cast, ExprNode, m, "_cast", DataType, Expr)
    REGISTER_CLASS(Get, ExprNode, m, "_get", Expr, size_t)
    REGISTER_CLASS(New, ExprNode, m, "_new", vector<Expr>)
    REGISTER_CLASS(StructGEP, ExprNode, m, "_structgep", Expr, size_t)

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
    REGISTER_CLASS(NaryExpr, ExprNode, m, "_nary", DataType, MathOp,
                   vector<Expr>)
    REGISTER_CLASS(UnaryExpr, NaryExpr, m, "_unary", DataType, MathOp, Expr)
    REGISTER_CLASS(BinaryExpr, NaryExpr, m, "_binary", DataType, MathOp, Expr,
                   Expr)

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
    REGISTER_CLASS(ForAll, NaryExpr, m, "_forall", Sym, Expr)
    REGISTER_CLASS(Exists, NaryExpr, m, "_exists", Sym, Expr)

    /* Comparison Operators*/
    REGISTER_CLASS(LessThan, BinaryExpr, m, "_lt", Expr, Expr)
    REGISTER_CLASS(GreaterThan, BinaryExpr, m, "_gt", Expr, Expr)
    REGISTER_CLASS(LessThanEqual, BinaryExpr, m, "_lte", Expr, Expr)
    REGISTER_CLASS(GreaterThanEqual, BinaryExpr, m, "_gte", Expr, Expr)

    /* CUDA */
    REGISTER_CLASS(ThreadIdx, ExprNode, m, "_tidx")
    REGISTER_CLASS(BlockIdx, ExprNode, m, "_bidx")
    REGISTER_CLASS(GridDim, ExprNode, m, "_gdim")
    REGISTER_CLASS(BlockDim, ExprNode, m, "_bdim")

    /* Constants */
    m.def("_i8", [](int8_t val) { return Const(types::INT8, val); });
    m.def("_i16", [](int16_t val) { return Const(types::INT16, val); });
    m.def("_i32", [](int32_t val) { return Const(types::INT32, val); });
    m.def("_i64", [](int64_t val) { return Const(types::INT64, val); });
    m.def("_u8", [](uint8_t val) { return Const(types::UINT8, val); });
    m.def("_u16", [](uint16_t val) { return Const(types::UINT16, val); });
    m.def("_u32", [](uint32_t val) { return Const(types::UINT32, val); });
    m.def("_u64", [](uint64_t val) { return Const(types::UINT64, val); });
    m.def("_f32", [](float val) { return Const(types::FLOAT32, val); });
    m.def("_f64", [](double val) { return Const(types::FLOAT64, val); });
    m.def("_ch", [](char val) { return Const(types::INT8, val); });
    m.def("_idx", [](int64_t val) { return Const(types::IDX, val); });
    m.def("_true", []() { return Const(types::BOOL, true); });
    m.def("_false", []() { return Const(types::BOOL, false); });

    /* Types */
    m.attr("_i8_t") = types::INT8;
    m.attr("_i16_t") = types::INT16;
    m.attr("_i32_t") = types::INT32;
    m.attr("_i64_t") = types::INT64;
    m.attr("_u8_t") = types::UINT8;
    m.attr("_u16_t") = types::UINT16;
    m.attr("_u32_t") = types::UINT32;
    m.attr("_u64_t") = types::UINT64;
    m.attr("_f32_t") = types::FLOAT32;
    m.attr("_f64_t") = types::FLOAT64;
    m.attr("_idx_t") = types::IDX;
    m.attr("_ch_t") = types::INT8;
    m.attr("_bool_t") = types::BOOL;
    m.def("STRUCT", [](std::vector<DataType> types) {
        return DataType(BaseType::STRUCT, types);
    });
    m.def("VECTOR", [](size_t dim, std::vector<DataType> types) {
        return DataType(BaseType::VECTOR, types, dim);
    });
}
