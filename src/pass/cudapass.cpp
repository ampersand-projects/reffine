#include <vector>
#include "llvm/IR/IntrinsicsNVPTX.h"

#include "reffine/pass/cudapass.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/builder/reffiner.h"

using namespace reffine;
using namespace reffine::reffiner;
using namespace llvm;


// void CUDAPass::Visit(Func& func)
// {
//     // for CUDA, output needs to be appended as an output
//     auto output_sym = _sym("output", func.output);
//     func.inputs.insert(func.inputs.begin(), output_sym);
//     // LLVMGen::Visit(func);
// }

shared_ptr<ExprNode> get_start_idx(Expr tid, Expr bid, Expr bdim, Expr gdim, Expr len) {
    // gridDim: # of blocks in a grid
    // blockDim: # of threads in a block
    
    auto tidx = _cast(_idx_t, tid);
    auto bidx = _cast(_idx_t, bid);
    auto bdimx = _cast(_idx_t, bdim);
    auto gdimx = _cast(_idx_t, gdim);
    auto n = _cast(_idx_t, len);

    auto elem_per_block = (n + gdimx - _idx(1)) / gdimx;
    auto block_start = bidx * elem_per_block;
    auto block_end = _min(block_start + elem_per_block, n);

    auto elem_per_thread = (block_end - block_start + bdimx - _idx(1)) / bdimx;
    auto thread_start = block_start + (tidx * elem_per_thread);

    return thread_start;
}

shared_ptr<ExprNode> get_end_idx(Expr tid, Expr bid, Expr bdim, Expr gdim, Expr len) {
    auto tidx = _cast(_idx_t, tid);
    auto bidx = _cast(_idx_t, bid);
    auto bdimx = _cast(_idx_t, bdim);
    auto gdimx = _cast(_idx_t, gdim);
    auto n = _cast(_idx_t, len);

    auto elem_per_block = (n + gdimx - _idx(1)) / gdimx;
    auto block_start = bidx * elem_per_block;
    auto block_end = _min(block_start + elem_per_block, n);

    auto elem_per_thread = (block_end - block_start + bdimx - _idx(1)) / bdimx;
    auto thread_start = block_start + (tidx * elem_per_thread);
    auto thread_end = _min(thread_start + elem_per_thread, block_end);

    return thread_end;
}

// void CUDAPass::Visit(Kernel& kernel)
// {
//     IRPass::Visit(kernel);
// }


void CUDAPass::Visit(Loop& loop)
{
    cout << "ENTERED CUDAPass LOOP VISIT" << endl << endl;
    auto tid = make_shared<ThreadIdx>();
    auto bid = make_shared<BlockIdx>();
    auto bdim = make_shared<BlockDim>();
    auto gdim = make_shared<GridDim>();

    auto n = _idx(10);  // TODO --> make this not hardcoded, use get_vector_len fcn or loop attribute

    auto idx_start = get_start_idx(tid, bid, bdim, gdim, n);
    auto idx_end = get_end_idx(tid, bid, bdim, gdim, n);

    // TODO: need a way to access address of index
        // from loop?
        // for now just create new one
    auto idx_alloc = _alloc(_idx_t);
    auto idx = _load(idx_alloc);

    // store start index
    auto idx_init = _store(idx_alloc, idx_start);
    loop.init = _stmts(vector<Stmt>{loop.init, idx_init});

    // add increment
    auto loop_incr = _store(idx_alloc, idx + _idx(1));
    loop.body = _stmts(vector<Stmt>{loop.body, loop_incr});

    // create exit_cond with index end
    loop.exit_cond = _gte(idx, idx_end);
}

void CUDAPass::Build(shared_ptr<Kernel> kernel, llvm::Module& llmod)
{
    llmod.setTargetTriple("nvptx64-nvidia-cuda");

    IRPassCtx ctx(kernel->tbl);
    CUDAPass pass(ctx);
    kernel->Accept(pass);
}