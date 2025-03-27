#include <vector>
#include "llvm/IR/IntrinsicsNVPTX.h"

#include "reffine/pass/cudagen.h"
#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;
using namespace llvm;


void CUDAGen::visit(Func& func)
{
    // for CUDA, output needs to be appended as an output
    auto output_sym = _sym("output", func.output);
    func.inputs.insert(func.inputs.begin(), output_sym);
    LLVMGen::Visit(func);
}

Value* CUDAGen::visit(ThreadIdx& tidx) {
    // https://llvm.org/docs/NVPTXUsage.html#overview
    auto thread_idx = builder()->CreateIntrinsic(
        Type::getInt32Ty(llctx()),
        llvm::Intrinsic::nvvm_read_ptx_sreg_tid_x,
        {}
    );

    return thread_idx;
}

Value* CUDAGen::visit(BlockIdx& bidx) {
    auto block_idx = builder()->CreateIntrinsic(
        Type::getInt32Ty(llctx()),
        llvm::Intrinsic::nvvm_read_ptx_sreg_ctaid_x,
        {}
    );

    return block_idx;
}

Value* CUDAGen::visit(BlockDim& bdim) {
    auto block_dim = builder()->CreateIntrinsic(
        Type::getInt32Ty(llctx()),
        llvm::Intrinsic::nvvm_read_ptx_sreg_ntid_x,
        {}
    );

    return block_dim;
}

Value* CUDAGen::visit(GridDim& bdim) {
    auto grid_dim = builder()->CreateIntrinsic(
        Type::getInt32Ty(llctx()),
        llvm::Intrinsic::nvvm_read_ptx_sreg_nctaid_x,
        {}
    );

    return grid_dim;
}

void CUDAGen::Build2(shared_ptr<Func> func, llvm::Module& llmod)
{
    llmod.setTargetTriple("nvptx64-nvidia-cuda");

    LLVMGenCtx ctx(func);
    LLVMGen cudagen(ctx, llmod);
    func->Accept(cudagen);
}