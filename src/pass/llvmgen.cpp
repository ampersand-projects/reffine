#include "reffine/base/type.h"
#include "reffine/pass/llvmgen.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/InstrTypes.h"

using namespace reffine;
using namespace llvm;

Function* LLVMGen::llfunc(const string name, llvm::Type* ret_type, vector<llvm::Type*> arg_types)
{
    auto fn_type = FunctionType::get(ret_type, arg_types, false);
    return Function::Create(fn_type, Function::ExternalLinkage, name, llmod());
}

Value* LLVMGen::llcall(const string name, llvm::Type* ret_type, vector<Value*> arg_vals)
{
    vector<llvm::Type*> arg_types;
    for (const auto& arg_val : arg_vals) {
        arg_types.push_back(arg_val->getType());
    }

    auto fn_type = FunctionType::get(ret_type, arg_types, false);
    auto fn = llmod()->getOrInsertFunction(name, fn_type);
    return builder()->CreateCall(fn, arg_vals);
}

Value* LLVMGen::llcall(const string name, llvm::Type* ret_type, vector<Expr> args)
{
    vector<Value*> arg_vals;
    for (const auto& arg : args) {
        arg_vals.push_back(eval(arg));
    }

    return llcall(name, ret_type, arg_vals);
}

llvm::Type* LLVMGen::lltype(const DataType& type)
{
    switch (type.btype) {
        case BaseType::BOOL:
            return llvm::Type::getInt1Ty(llctx());
        case BaseType::INT8:
        case BaseType::UINT8:
            return llvm::Type::getInt8Ty(llctx());
        case BaseType::INT16:
        case BaseType::UINT16:
            return llvm::Type::getInt16Ty(llctx());
        case BaseType::INT32:
        case BaseType::UINT32:
            return llvm::Type::getInt32Ty(llctx());
        case BaseType::INT64:
        case BaseType::UINT64:
            return llvm::Type::getInt64Ty(llctx());
        case BaseType::FLOAT32:
            return llvm::Type::getFloatTy(llctx());
        case BaseType::FLOAT64:
            return llvm::Type::getDoubleTy(llctx());
        case BaseType::STRUCT: {
            vector<llvm::Type*> lltypes;
            for (auto dt : type.dtypes) {
                lltypes.push_back(lltype(dt));
            }
            return StructType::get(llctx(), lltypes);
        }
        case BaseType::PTR:
            return PointerType::get(lltype(type.dtypes[0]), 0);
        case BaseType::UNKNOWN:
        default:
            throw std::runtime_error("Invalid type");
    }
}

Value* LLVMGen::visit(const Const& cnst)
{
    switch (cnst.type.btype) {
        case BaseType::BOOL:
        case BaseType::INT8:
        case BaseType::INT16:
        case BaseType::INT32:
        case BaseType::INT64:
        case BaseType::UINT8:
        case BaseType::UINT16:
        case BaseType::UINT32:
        case BaseType::UINT64:
        case BaseType::IDX: return ConstantInt::get(lltype(cnst), cnst.val);
        case BaseType::FLOAT32:
        case BaseType::FLOAT64: return ConstantFP::get(lltype(cnst), cnst.val);
        default: throw std::runtime_error("Invalid constant type"); break;
    }
}

Value* LLVMGen::visit(const Cast& e)
{
    auto input_val = eval(e.arg);
    auto dest_type = lltype(e);
    auto op = CastInst::getCastOpcode(input_val, e.arg->type.is_signed(), dest_type, e.type.is_signed());
    return builder()->CreateCast(op, input_val, dest_type);
}

Value* LLVMGen::visit(const NaryExpr& e)
{
    switch (e.op) {
        case MathOp::ADD: {
            if (e.type.is_float()) {
                return builder()->CreateFAdd(eval(e.arg(0)), eval(e.arg(1)));
            } else {
                return builder()->CreateAdd(eval(e.arg(0)), eval(e.arg(1)));
            }
        }
        case MathOp::SUB: {
            if (e.type.is_float()) {
                return builder()->CreateFSub(eval(e.arg(0)), eval(e.arg(1)));
            } else {
                return builder()->CreateSub(eval(e.arg(0)), eval(e.arg(1)));
            }
        }
        case MathOp::MUL: {
            if (e.type.is_float()) {
                return builder()->CreateFMul(eval(e.arg(0)), eval(e.arg(1)));
            } else {
                return builder()->CreateMul(eval(e.arg(0)), eval(e.arg(1)));
            }
        }
        case MathOp::DIV: {
            if (e.type.is_float()) {
                return builder()->CreateFDiv(eval(e.arg(0)), eval(e.arg(1)));
            } else if (e.type.is_signed()) {
                return builder()->CreateSDiv(eval(e.arg(0)), eval(e.arg(1)));
            } else {
                return builder()->CreateUDiv(eval(e.arg(0)), eval(e.arg(1)));
            }
        }
        case MathOp::MAX: {
            auto left = eval(e.arg(0));
            auto right = eval(e.arg(1));

            Value* cond;
            if (e.type.is_float()) {
                cond = builder()->CreateFCmpOGE(left, right);
            } else if (e.type.is_signed()) {
                cond = builder()->CreateICmpSGE(left, right);
            } else {
                cond = builder()->CreateICmpUGE(left, right);
            }
            return builder()->CreateSelect(cond, left, right);
        }
        case MathOp::MIN: {
            auto left = eval(e.arg(0));
            auto right = eval(e.arg(1));

            Value* cond;
            if (e.type.is_float()) {
                cond = builder()->CreateFCmpOLE(left, right);
            } else if (e.type.is_signed()) {
                cond = builder()->CreateICmpSLE(left, right);
            } else {
                cond = builder()->CreateICmpULE(left, right);
            }
            return builder()->CreateSelect(cond, left, right);
        }
        case MathOp::MOD: {
            if (e.type.is_signed()) {
                return builder()->CreateSRem(eval(e.arg(0)), eval(e.arg(1)));
            } else {
                return builder()->CreateURem(eval(e.arg(0)), eval(e.arg(1)));
            }
        }
        case MathOp::ABS: {
            auto input = eval(e.arg(0));

            if (e.type.is_float()) {
                return builder()->CreateIntrinsic(Intrinsic::fabs, {lltype(e.arg(0))}, {input});
            } else {
                auto neg = builder()->CreateNeg(input);

                Value* cond;
                if (e.type.is_signed()) {
                    cond = builder()->CreateICmpSGE(input, ConstantInt::get(lltype(types::INT32), 0));
                } else {
                    cond = builder()->CreateICmpUGE(input, ConstantInt::get(lltype(types::UINT32), 0));
                }
                return builder()->CreateSelect(cond, input, neg);
            }
        }
        case MathOp::NEG: {
            if (e.type.is_float()) {
                return builder()->CreateFNeg(eval(e.arg(0)));
            } else {
                return builder()->CreateNeg(eval(e.arg(0)));
            }
        }
        case MathOp::SQRT: return builder()->CreateIntrinsic(Intrinsic::sqrt, {lltype(e.arg(0))}, {eval(e.arg(0))});
        case MathOp::POW: return builder()->CreateIntrinsic(
            Intrinsic::pow, {lltype(e.arg(0))}, {eval(e.arg(0)), eval(e.arg(1))});
        case MathOp::CEIL: return builder()->CreateIntrinsic(Intrinsic::ceil, {lltype(e.arg(0))}, {eval(e.arg(0))});
        case MathOp::FLOOR: return builder()->CreateIntrinsic(Intrinsic::floor, {lltype(e.arg(0))}, {eval(e.arg(0))});
        case MathOp::EQ: {
            if (e.arg(0)->type.is_float()) {
                return builder()->CreateFCmpOEQ(eval(e.arg(0)), eval(e.arg(1)));
            } else {
                return builder()->CreateICmpEQ(eval(e.arg(0)), eval(e.arg(1)));
            }
        }
        case MathOp::LT: {
            if (e.arg(0)->type.is_float()) {
                return builder()->CreateFCmpOLT(eval(e.arg(0)), eval(e.arg(1)));
            } else if (e.arg(0)->type.is_signed()) {
                return builder()->CreateICmpSLT(eval(e.arg(0)), eval(e.arg(1)));
            } else {
                return builder()->CreateICmpULT(eval(e.arg(0)), eval(e.arg(1)));
            }
        }
        case MathOp::LTE: {
            if (e.arg(0)->type.is_float()) {
                return builder()->CreateFCmpOLE(eval(e.arg(0)), eval(e.arg(1)));
            } else if (e.arg(0)->type.is_signed()) {
                return builder()->CreateICmpSLE(eval(e.arg(0)), eval(e.arg(1)));
            } else {
                return builder()->CreateICmpULE(eval(e.arg(0)), eval(e.arg(1)));
            }
        }
        case MathOp::GT: {
            if (e.arg(0)->type.is_float()) {
                return builder()->CreateFCmpOGT(eval(e.arg(0)), eval(e.arg(1)));
            } else if (e.arg(0)->type.is_signed()) {
                return builder()->CreateICmpSGT(eval(e.arg(0)), eval(e.arg(1)));
            } else {
                return builder()->CreateICmpUGT(eval(e.arg(0)), eval(e.arg(1)));
            }
        }
        case MathOp::GTE: {
            if (e.arg(0)->type.is_float()) {
                return builder()->CreateFCmpOGE(eval(e.arg(0)), eval(e.arg(1)));
            } else if (e.arg(0)->type.is_signed()) {
                return builder()->CreateICmpSGE(eval(e.arg(0)), eval(e.arg(1)));
            } else {
                return builder()->CreateICmpUGE(eval(e.arg(0)), eval(e.arg(1)));
            }
        }
        case MathOp::NOT: return builder()->CreateNot(eval(e.arg(0)));
        case MathOp::AND: return builder()->CreateAnd(eval(e.arg(0)), eval(e.arg(1)));
        case MathOp::OR: return builder()->CreateOr(eval(e.arg(0)), eval(e.arg(1)));
        default: throw std::runtime_error("Invalid math operation"); break;
    }
}

Value* LLVMGen::visit(const Select&) { return nullptr; }
Value* LLVMGen::visit(const IfElse&) { return nullptr; }
Value* LLVMGen::visit(const Read&) { return nullptr; }
Value* LLVMGen::visit(const PushBack&) { return nullptr; }
Value* LLVMGen::visit(const Stmts&) { return nullptr; }
Value* LLVMGen::visit(const Func&) { return nullptr; }
Value* LLVMGen::visit(const Loop&) { return nullptr; }

Value* LLVMGen::visit(const Exists& exists)
{
    return builder()->CreateIsNotNull(eval(exists.sym));
}

Value* LLVMGen::visit(const Call& call)
{
    return llcall(call.name, lltype(call), call.args);
}

void LLVMGen::Build(const Func* func, llvm::Module& llmod)
{
    LLVMGenCtx ctx(func);
    LLVMGen llgen(std::move(ctx), llmod);
    func->Accept(llgen);
}
