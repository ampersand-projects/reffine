#include <cuda.h>
#include <cuda_runtime.h>

#include "reffine/builder/reffiner.h"
#include "reffine/engine/cuda_engine.h"
#include "test_base.h"
#include "test_utils.h"

using namespace reffine;
using namespace reffine::reffiner;

shared_ptr<Func> aggregate_kernel(int n)
{
    auto sum_out_sym = _sym("res", types::INT64.ptr());
    auto vec_in_sym = _sym("input", types::INT64.ptr());

    auto len = _idx(n);
    auto idx_alloc = _alloc(_idx_t);
    auto idx_addr = _sym("idx_addr", idx_alloc);
    auto idx = _load(idx_addr);

    auto temp_alloc = _alloc(_i64_t);
    auto temp_addr = _sym("temp_addr", temp_alloc);
    auto temp_sum = _load(temp_addr);

    auto val_ptr = _call("get_elem_ptr", types::INT64.ptr(),
                         vector<Expr>{vec_in_sym, idx});
    auto val = _load(val_ptr);

    auto loop = _loop(_load(sum_out_sym));

    loop->init = _stmts(vector<Stmt>{
        idx_alloc,
        _store(idx_addr, _idx(0)),
        temp_alloc,
        _store(temp_addr, _i64(0)),
    });
    loop->body = _stmts(vector<Stmt>{
        _store(temp_addr, _add(temp_sum, val)),
        _store(idx_addr, idx + _idx(1)),
    });
    loop->exit_cond = _gte(idx, len);
    loop->post = _atomic_add(sum_out_sym, temp_sum);
    auto loop_sym = _sym("loop", loop);

    auto foo_fn = make_shared<Func>("foo", _stmtexpr(loop_sym),
                                    vector<Sym>{sum_out_sym, vec_in_sym},
                                    SymTable(), true);
    foo_fn->tbl[idx_addr] = idx_alloc;
    foo_fn->tbl[temp_addr] = temp_alloc;
    foo_fn->tbl[loop_sym] = loop;

    return foo_fn;
}

void test_aggregate_kernel(int len, int expected_res)
{
    /* Test kernel generation and execution*/
    auto fn = aggregate_kernel(len);
    CanonPass::Build(fn);
    cout << "Loop IR (canon):" << endl << IRPrinter::Build(fn) << endl;

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("foo", jit->GetCtx());
    LLVMGen::Build(fn, *llmod);
    jit->Optimize(*llmod);

    auto cuda_engine = CudaEngine::Get();
    auto cuda_module = cuda_engine->Build(*llmod);
    auto kernel = cuda_engine->Lookup(cuda_module, llmod->getName().str());

    int64_t* in_array = new int64_t[len];
    for (int i = 0; i < len; i++) { in_array[i] = i; }

    CUdeviceptr d_arr;
    cuMemAlloc(&d_arr, sizeof(int64_t) * len);
    cuMemcpyHtoD(d_arr, in_array, sizeof(int64_t) * len);

    int res = 0;
    CUdeviceptr d_res_out;
    cuMemAlloc(&d_res_out, sizeof(int64_t));
    cuMemcpyHtoD(d_res_out, &res, sizeof(int64_t));

    void* kernelParams[] = {
        &d_res_out,
        &d_arr,
    };

    cuLaunchKernel(kernel, 1, 1, 1, 1, 1, 1,
                   0,  // shared memory size
                   0,  // stream handle
                   kernelParams, NULL);

    cuCtxSynchronize();

    cuMemcpyDtoH(&res, d_res_out, sizeof(int64_t));
    cuMemFree(d_arr);

    ASSERT_EQ(res, expected_res);

    cuMemFree(d_arr);
    cuMemFree(d_res_out);

    cuda_engine->Cleanup(cuda_module);

    return;
}

void cuda_test() { test_aggregate_kernel(128, 8128); }