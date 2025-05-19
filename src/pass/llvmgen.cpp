#include "reffine/pass/llvmgen.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "reffine/base/type.h"
#ifdef ENABLE_CUDA
#include "llvm/IR/IntrinsicsNVPTX.h"
#endif

using namespace reffine;
using namespace llvm;

Function* LLVMGen::llfunc(const string name, llvm::Type* ret_type,
                          vector<llvm::Type*> arg_types)
{
    auto fn_type = FunctionType::get(ret_type, arg_types, false);
    return Function::Create(fn_type, Function::ExternalLinkage, name, llmod());
}

Value* LLVMGen::llcall(const string name, llvm::Type* ret_type,
                       vector<Value*> arg_vals)
{
    vector<llvm::Type*> arg_types;
    for (const auto& arg_val : arg_vals) {
        arg_types.push_back(arg_val->getType());
    }

    auto fn_type = FunctionType::get(ret_type, arg_types, false);
    auto fn = llmod()->getOrInsertFunction(name, fn_type);
    return builder()->CreateCall(fn, arg_vals);
}

Value* LLVMGen::llcall(const string name, llvm::Type* ret_type,
                       vector<Expr> args)
{
    vector<Value*> arg_vals;
    for (const auto& arg : args) { arg_vals.push_back(eval(arg)); }

    return llcall(name, ret_type, arg_vals);
}

llvm::Type* LLVMGen::lltype(const DataType& type)
{
    switch (type.btype) {
        case BaseType::VOID:
            return llvm::Type::getVoidTy(llctx());
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
        case BaseType::IDX:
            return llvm::Type::getInt64Ty(llctx());
        case BaseType::FLOAT32:
            return llvm::Type::getFloatTy(llctx());
        case BaseType::FLOAT64:
            return llvm::Type::getDoubleTy(llctx());
        case BaseType::STRUCT: {
            vector<llvm::Type*> lltypes;
            for (auto dt : type.dtypes) { lltypes.push_back(lltype(dt)); }
            return StructType::get(llctx(), lltypes);
        }
        case BaseType::PTR:
            return PointerType::get(lltype(type.dtypes[0]), 0);
        case BaseType::VECTOR:
            return PointerType::get(
                llvm::StructType::getTypeByName(llctx(), "struct.ArrowArray"),
                0);
        case BaseType::UNKNOWN:
        default:
            throw std::runtime_error("Invalid type");
    }
}

Value* LLVMGen::visit(Const& cnst)
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
        case BaseType::IDX:
            return ConstantInt::get(lltype(cnst), cnst.val);
        case BaseType::FLOAT32:
        case BaseType::FLOAT64:
            return ConstantFP::get(lltype(cnst), cnst.val);
        default:
            throw std::runtime_error("Invalid constant type");
            break;
    }
}

Value* LLVMGen::visit(Cast& e)
{
    auto input_val = eval(e.arg);
    auto dest_type = lltype(e);
    auto op = CastInst::getCastOpcode(input_val, e.arg->type.is_signed(),
                                      dest_type, e.type.is_signed());
    return builder()->CreateCast(op, input_val, dest_type);
}

Value* LLVMGen::visit(Get& e)
{
    auto val = eval(e.val);
    return builder()->CreateExtractValue(val, e.col);
}

Value* LLVMGen::visit(New& e)
{
    auto new_type = lltype(e);
    auto ptr = CreateAlloca(new_type);

    for (size_t i = 0; i < e.vals.size(); i++) {
        auto val_ptr = builder()->CreateStructGEP(new_type, ptr, i);
        CreateStore(eval(e.vals[i]), val_ptr);
    }

    return CreateLoad(new_type, ptr);
}

Value* LLVMGen::visit(NaryExpr& e)
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
                return builder()->CreateIntrinsic(Intrinsic::fabs,
                                                  {lltype(e.arg(0))}, {input});
            } else {
                auto neg = builder()->CreateNeg(input);

                Value* cond;
                if (e.type.is_signed()) {
                    cond = builder()->CreateICmpSGE(
                        input, ConstantInt::get(lltype(types::INT32), 0));
                } else {
                    cond = builder()->CreateICmpUGE(
                        input, ConstantInt::get(lltype(types::UINT32), 0));
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
        case MathOp::SQRT:
            return builder()->CreateIntrinsic(
                Intrinsic::sqrt, {lltype(e.arg(0))}, {eval(e.arg(0))});
        case MathOp::POW:
            return builder()->CreateIntrinsic(Intrinsic::pow,
                                              {lltype(e.arg(0))},
                                              {eval(e.arg(0)), eval(e.arg(1))});
        case MathOp::CEIL:
            return builder()->CreateIntrinsic(
                Intrinsic::ceil, {lltype(e.arg(0))}, {eval(e.arg(0))});
        case MathOp::FLOOR:
            return builder()->CreateIntrinsic(
                Intrinsic::floor, {lltype(e.arg(0))}, {eval(e.arg(0))});
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
        case MathOp::NOT:
            return builder()->CreateNot(eval(e.arg(0)));
        case MathOp::AND:
            return builder()->CreateAnd(eval(e.arg(0)), eval(e.arg(1)));
        case MathOp::OR:
            return builder()->CreateOr(eval(e.arg(0)), eval(e.arg(1)));
        default:
            throw std::runtime_error("Invalid math operation");
            break;
    }
}

Value* LLVMGen::visit(Select& select)
{
    auto cond = eval(select.cond);
    auto true_val = eval(select.true_body);
    auto false_val = eval(select.false_body);
    return builder()->CreateSelect(cond, true_val, false_val);
}

Value* LLVMGen::visit(IfElse& ifelse)
{
    auto parent_fn = builder()->GetInsertBlock()->getParent();
    auto then_bb = BasicBlock::Create(llctx(), "then");
    auto else_bb = BasicBlock::Create(llctx(), "else");
    auto merge_bb = BasicBlock::Create(llctx(), "merge");

    // condition check
    auto cond = eval(ifelse.cond);
    builder()->CreateCondBr(cond, then_bb, else_bb);

    // then block
    parent_fn->insert(parent_fn->end(), then_bb);
    builder()->SetInsertPoint(then_bb);
    eval(ifelse.true_body);
    then_bb = builder()->GetInsertBlock();
    builder()->CreateBr(merge_bb);

    // else block
    parent_fn->insert(parent_fn->end(), else_bb);
    builder()->SetInsertPoint(else_bb);
    eval(ifelse.false_body);
    else_bb = builder()->GetInsertBlock();
    builder()->CreateBr(merge_bb);

    // merge block
    parent_fn->insert(parent_fn->end(), merge_bb);
    builder()->SetInsertPoint(merge_bb);

    return nullptr;
}

Value* LLVMGen::visit(NoOp&) { return nullptr; }

Value* LLVMGen::visit(IsValid& is_valid)
{
    auto vec_val = eval(is_valid.vec);
    auto idx_val = eval(is_valid.idx);
    auto col_val = ConstantInt::get(lltype(types::UINT32), is_valid.col);

    return llcall("get_vector_null_bit", lltype(is_valid),
                  {vec_val, idx_val, col_val});
}

Value* LLVMGen::visit(SetValid& set_valid)
{
    auto vec_val = eval(set_valid.vec);
    auto idx_val = eval(set_valid.idx);
    auto validity_val = eval(set_valid.validity);
    auto col_val = ConstantInt::get(lltype(types::UINT32), set_valid.col);

    return llcall("set_vector_null_bit", lltype(set_valid),
                  {vec_val, idx_val, validity_val, col_val});
}

Value* LLVMGen::visit(FetchDataPtr& fetch_data_ptr)
{
    auto vec_val = eval(fetch_data_ptr.vec);
    auto idx_val = eval(fetch_data_ptr.idx);
    auto col_val = ConstantInt::get(lltype(types::UINT32), fetch_data_ptr.col);

    auto buf_addr = llcall("get_vector_data_buf", lltype(fetch_data_ptr),
                           {vec_val, col_val});
    auto data_addr = builder()->CreateGEP(lltype(fetch_data_ptr.type.deref()),
                                          buf_addr, idx_val);

    return data_addr;
}

Value* LLVMGen::visit(Call& call)
{
    return llcall(call.name, lltype(call), call.args);
}

Value* LLVMGen::visit(Stmts& stmts)
{
    for (auto& stmt : stmts.stmts) { eval(stmt); }

    return nullptr;
}

Value* LLVMGen::visit(Alloc& alloc)
{
    return CreateAlloca(lltype(alloc.type), eval(alloc.size));
}

Value* LLVMGen::visit(Load& load)
{
    auto addr = eval(load.addr);
    auto addr_type = lltype(load.addr->type.deref());
    return CreateLoad(addr_type, addr);
}

Value* LLVMGen::visit(Store& store)
{
    auto addr = eval(store.addr);
    auto val = eval(store.val);
    return CreateStore(val, addr);
}

Value* LLVMGen::visit(ThreadIdx& tidx)
{
    // https://llvm.org/docs/NVPTXUsage.html#overview

#ifdef ENABLE_CUDA
    auto thread_idx = builder()->CreateIntrinsic(
        lltype(tidx), llvm::Intrinsic::nvvm_read_ptx_sreg_tid_x, {});

    return thread_idx;
#else
    throw std::runtime_error("CUDA not enabled.");
#endif
}

Value* LLVMGen::visit(BlockIdx& bidx)
{
#ifdef ENABLE_CUDA
    auto block_idx = builder()->CreateIntrinsic(
        lltype(bidx), llvm::Intrinsic::nvvm_read_ptx_sreg_ctaid_x, {});

    return block_idx;
#else
    throw std::runtime_error("CUDA not enabled.");
#endif
}

Value* LLVMGen::visit(BlockDim& bdim)
{
#ifdef ENABLE_CUDA
    auto block_dim = builder()->CreateIntrinsic(
        lltype(bdim), llvm::Intrinsic::nvvm_read_ptx_sreg_ntid_x, {});

    return block_dim;
#else
    throw std::runtime_error("CUDA not enabled.");
#endif
}

Value* LLVMGen::visit(GridDim& gdim)
{
#ifdef ENABLE_CUDA
    auto grid_dim = builder()->CreateIntrinsic(
        lltype(gdim), llvm::Intrinsic::nvvm_read_ptx_sreg_nctaid_x, {});

    return grid_dim;
#else
    throw std::runtime_error("CUDA not enabled.");
#endif
}

Value* LLVMGen::visit(Loop& loop)
{
    // Loop body condition and incr needs to be merged into loop body before
    // code generation
    ASSERT(loop.body_cond == nullptr);
    ASSERT(loop.incr == nullptr);
    // Loop must have a body
    ASSERT(loop.body != nullptr);
    // Loop must have an exit condition
    ASSERT(loop.exit_cond != nullptr);

    auto parent_fn = builder()->GetInsertBlock()->getParent();
    auto preheader_bb = BasicBlock::Create(llctx(), "preheader");
    auto header_bb = BasicBlock::Create(llctx(), "header");
    auto body_bb = BasicBlock::Create(llctx(), "body");
    auto exit_bb = BasicBlock::Create(llctx(), "exit");

    builder()->CreateBr(preheader_bb);

    // initialize loop
    parent_fn->insert(parent_fn->end(), preheader_bb);
    builder()->SetInsertPoint(preheader_bb);
    if (loop.init) { eval(loop.init); }
    builder()->CreateBr(header_bb);

    // loop exit condition
    parent_fn->insert(parent_fn->end(), header_bb);
    builder()->SetInsertPoint(header_bb);
    builder()->CreateCondBr(eval(loop.exit_cond), exit_bb, body_bb);

    // loop body
    parent_fn->insert(parent_fn->end(), body_bb);
    builder()->SetInsertPoint(body_bb);
    eval(loop.body);

    // Jump back to loop header
    builder()->CreateBr(header_bb);

    // loop post and exit
    parent_fn->insert(parent_fn->end(), exit_bb);
    builder()->SetInsertPoint(exit_bb);
    if (loop.post) { eval(loop.post); }

    return eval(loop.output);
}

llvm::Value* LLVMGen::visit(Func& func)
{
    ASSERT(func.output->type.is_void());

    // Define function signature
    vector<llvm::Type*> args_type;
    for (auto& input : func.inputs) {
        args_type.push_back(lltype(input->type));
    }
    llvm::Type* ret_type = nullptr;
    if (func.is_kernel) {
        ret_type = llvm::Type::getVoidTy(llctx());
    } else {
        ret_type = lltype(func.output);
    }
    auto fn = llfunc(func.name, ret_type, args_type);
    for (size_t i = 0; i < func.inputs.size(); i++) {
        auto input = func.inputs[i];
        fn->getArg(i)->setName(input->name);
        fn->getArg(i)->addAttr(Attribute::NoAlias);
        this->assign(input, fn->getArg(i));
    }

    auto entry_bb = BasicBlock::Create(llctx(), "entry", fn);

    builder()->SetInsertPoint(entry_bb);

    eval(func.output);

    if (func.is_kernel) {
#ifdef ENABLE_CUDA
        fn->setCallingConv(llvm::CallingConv::PTX_Kernel);
        llvm::NamedMDNode* MD =
            llmod()->getOrInsertNamedMetadata("nvvm.annotations");

        std::vector<llvm::Metadata*> MDVals;
        MDVals.push_back(llvm::ValueAsMetadata::get(fn));
        MDVals.push_back(llvm::MDString::get(llctx(), "kernel"));
        MDVals.push_back(llvm::ConstantAsMetadata::get(
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(llctx()), 1)));

        MD->addOperand(llvm::MDNode::get(llctx(), MDVals));
#else
        throw std::runtime_error("CUDA not enabled.");
#endif
    }

    builder()->CreateRetVoid();

    return fn;
}

void LLVMGen::register_vinstrs()
{
    const auto buffer =
        llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(vinstr_str));

    llvm::SMDiagnostic error;
    std::unique_ptr<llvm::Module> vinstr_mod =
        llvm::parseIR(*buffer, error, llctx());
    if (!vinstr_mod) {
        throw std::runtime_error("Failed to parse vinstr bitcode");
    }
    if (llvm::verifyModule(*vinstr_mod)) {
        throw std::runtime_error("Failed to verify vinstr module");
    }

    // For some reason if we try to set internal linkage before we link
    // modules, then the JIT will be unable to find the symbols.
    // Instead we collect the function names first, then add internal
    // linkage to them after linking the modules
    std::vector<string> vinstr_names;
    for (const auto& function : vinstr_mod->functions()) {
        if (function.isDeclaration()) { continue; }
        vinstr_names.push_back(function.getName().str());
    }

    llvm::Linker::linkModules(*llmod(), std::move(vinstr_mod));
    for (const auto& name : vinstr_names) {
        llmod()
            ->getFunction(name.c_str())
            ->setLinkage(llvm::Function::InternalLinkage);
    }
}

llvm::Value* LLVMGen::visit(Sym old_sym)
{
    auto old_val = this->ctx().in_sym_tbl.at(old_sym);
    auto new_val = eval(old_val);

    auto var_addr = CreateAlloca(new_val->getType());
    var_addr->setName(old_sym->name + "_ref");
    CreateStore(new_val, var_addr);
    auto var = CreateLoad(lltype(old_sym), var_addr);
    var->setName(old_sym->name);

    return var;
}

void LLVMGen::Build(shared_ptr<Func> func, llvm::Module& llmod)
{
    LLVMGenCtx ctx(func);
    LLVMGen llgen(ctx, llmod);
    func->Accept(llgen);
}

// Helpers
llvm::StoreInst* LLVMGen::CreateStore(Value* new_val, Value* var_addr)
{
    MDNode* md_node = MDNode::get(llctx(), ArrayRef<Metadata*>());
    auto store = builder()->CreateStore(new_val, var_addr);
    store->setMetadata(LLVMContext::MD_noalias, md_node);

    return store;
}

llvm::LoadInst* LLVMGen::CreateLoad(Type* type, Value* addr)
{
    MDNode* md_node = MDNode::get(llctx(), ArrayRef<Metadata*>());
    auto var = builder()->CreateLoad(type, addr);
    var->setMetadata(LLVMContext::MD_noalias, md_node);

    return var;
}

llvm::AllocaInst* LLVMGen::CreateAlloca(llvm::Type* type, llvm::Value* size)
{
    // 5 for local address space
    // see https://llvm.org/docs/NVPTXUsage.html#address-spaces
    return builder()->CreateAlloca(type, 5U, size);
}
