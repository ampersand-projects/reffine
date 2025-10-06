#include <iostream>
#include <memory>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <sys/resource.h>

#include <z3++.h>

#ifdef ENABLE_CUDA
#include <cuda.h>
#include <cuda_runtime.h>
#endif

#include "reffine/ir/node.h"
#include "reffine/ir/stmt.h"
#include "reffine/ir/expr.h"
#include "reffine/ir/loop.h"
#include "reffine/ir/op.h"
#include "reffine/base/type.h"
#include "reffine/pass/printer2.h"
#include "reffine/pass/cemitter.h"
#include "reffine/pass/canonpass.h"
#include "reffine/pass/scalarpass.h"
#include "reffine/pass/symanalysis.h"
#include "reffine/pass/reffinepass.h"
#include "reffine/pass/loopgen.h"
#include "reffine/pass/z3solver.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/engine/engine.h"
#include "reffine/engine/cuda_engine.h"
#include "reffine/arrow/table.h"
#include "reffine/builder/reffiner.h"
#include "reffine/utils/utils.h"

#include <arrow/api.h>
#include <arrow/csv/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/c/bridge.h>

using namespace reffine;
using namespace std;
using namespace reffine::reffiner;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::microseconds;

arrow::Status csv_to_arrow()
{
    std::shared_ptr<arrow::io::ReadableFile> infile;
    ARROW_ASSIGN_OR_RAISE(infile, arrow::io::ReadableFile::Open("../students.csv"));

    std::shared_ptr<arrow::Table> csv_table;
    ARROW_ASSIGN_OR_RAISE(auto csv_reader, arrow::csv::TableReader::Make(
        arrow::io::default_io_context(), infile, arrow::csv::ReadOptions::Defaults(),
        arrow::csv::ParseOptions::Defaults(), arrow::csv::ConvertOptions::Defaults()));
    ARROW_ASSIGN_OR_RAISE(csv_table, csv_reader->Read());

    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    ARROW_ASSIGN_OR_RAISE(outfile, arrow::io::FileOutputStream::Open("../students.arrow"));
    ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::ipc::RecordBatchWriter> ipc_writer,
            arrow::ipc::MakeFileWriter(outfile, csv_table->schema()));
    ARROW_RETURN_NOT_OK(ipc_writer->WriteTable(*csv_table));
    ARROW_RETURN_NOT_OK(ipc_writer->Close());

    return arrow::Status::OK();
}

arrow::Result<shared_ptr<ArrowTable2>> load_arrow_file(string filename)
{
    ARROW_ASSIGN_OR_RAISE(auto file, arrow::io::ReadableFile::Open(
                filename, arrow::default_memory_pool()));

    ARROW_ASSIGN_OR_RAISE(auto ipc_reader, arrow::ipc::RecordBatchFileReader::Open(file));

    ARROW_ASSIGN_OR_RAISE(auto rbatch, ipc_reader->ReadRecordBatch(0));
    cout << "Input: " << endl << rbatch->ToString() << endl;

    auto tbl = make_shared<ArrowTable2>();
    ARROW_RETURN_NOT_OK(arrow::ExportRecordBatch(*rbatch, tbl->array, tbl->schema));

    return tbl;
}


arrow::Status query_arrow_file(shared_ptr<ArrowTable> in_table, void (*query_fn)(void*, void*))
{
    long sum;
    auto t1 = high_resolution_clock::now();
    query_fn(&sum, in_table.get());
    auto t2 = high_resolution_clock::now();
    cout << "SUM: " << sum << endl;

    auto us_int = duration_cast<microseconds>(t2 - t1);
    std::cout << "Time: " << us_int.count() << "us\n";

    return arrow::Status::OK();
}

arrow::Status query_arrow_file2(shared_ptr<ArrowTable> in_table, void (*query_fn)(void*, void*))
{
    ArrowTable* out_table;

    auto t1 = high_resolution_clock::now();
    query_fn(&out_table, in_table.get());
    auto t2 = high_resolution_clock::now();

    ARROW_ASSIGN_OR_RAISE(auto res, arrow::ImportRecordBatch(out_table->array, out_table->schema));
    cout << "Output: " << endl << res->ToString() << endl;

    auto us_int = duration_cast<microseconds>(t2 - t1);
    std::cout << "Time: " << us_int.count() << "us\n";

    return arrow::Status::OK();
}

shared_ptr<ExprNode> get_start_idx(Expr tid, Expr bid, Expr bdim, Expr gdim, Expr len) {
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

shared_ptr<Func> basic_transform_kernel()
{
    /* kernel version of transform_fn */
    auto vec_out_sym = _sym("res", types::INT64.ptr());
    auto vec_in_sym = _sym("input", types::INT64.ptr());

    auto len = _idx(1024);
    auto idx_alloc = _alloc(_idx_t);
    auto idx_addr = _sym("idx_addr", idx_alloc);
    auto idx = _load(idx_addr);

    auto val_ptr = _call("get_elem_ptr", types::INT64.ptr(), vector<Expr>{vec_in_sym, idx});
    auto val = _load(val_ptr);

    auto out_ptr = _call("get_elem_ptr", types::INT64.ptr(), vector<Expr>{vec_out_sym, idx});

    auto idx_start = get_start_idx(_tidx(), _bidx(), _bdim(), _gdim(), len);
    auto idx_end = get_end_idx(_tidx(), _bidx(), _bdim(), _gdim(), len);

    auto loop = _loop(_load(vec_out_sym));

    loop->init = _stmts(vector<Expr>{
        idx_alloc,
        _store(idx_addr, idx_start),
    });
    loop->body = _stmts(vector<Expr>{
        _store(out_ptr, _add(_i64(1), val)),
        _store(idx_addr, idx + _idx(1)),
    });
    loop->exit_cond = _gte(idx, idx_end);
    auto loop_sym = _sym("loop", loop);

    auto foo_fn = make_shared<Func>("foo", loop, vector<Sym>{vec_out_sym, vec_in_sym}, SymTable(), true);
    foo_fn->tbl[idx_addr] = idx_alloc;

    return foo_fn;
}

shared_ptr<Func> test_op_fn()
{
    auto t_sym = make_shared<SymNode>("t", types::INT32);
    Op op(
        vector<Sym>{t_sym},
        make_shared<And>(
            make_shared<GreaterThan>(t_sym, make_shared<Const>(types::INT32, 0)),
            make_shared<LessThan>(t_sym, make_shared<Const>(types::INT32, 10))
        ),
        vector<Expr>{t_sym}
    );

    auto sum = make_shared<Reduce>(
        op,
        [] () { return make_shared<Const>(types::INT32, 0); },
        [] (Expr s, Expr v) {
            auto e = make_shared<Get>(v, 0);
            return make_shared<Add>(s, e);
        }
    );
    auto sum_sym = make_shared<SymNode>("sum", sum);

    auto foo_fn = make_shared<Func>("foo", sum_sym, vector<Sym>{});
    foo_fn->tbl[sum_sym] = sum;

    return foo_fn;
}

#ifdef ENABLE_CUDA
void execute_kernel(string kernel_name, CUfunction kernel, void *arg, int len)
{
    CUdeviceptr d_arr;
    cuMemAlloc(&d_arr, sizeof(int64_t) * len);
    cuMemcpyHtoD(d_arr, arg, sizeof(int64_t) * len);

    CUdeviceptr d_arr_out;
    cuMemAlloc(&d_arr_out, sizeof(int64_t) * len);

    cout << "About to run " << kernel_name << " kernel..." << endl;

    int blockDimX = 32;  // num of threads per block
    int gridDimX = (len + blockDimX - 1) / blockDimX;  // num of blocks
    void *kernelParams[] = {
        &d_arr_out,
        &d_arr,
    };

    cuLaunchKernel(kernel, gridDimX, 1, 1, blockDimX, 1, 1,
                                   0,  // shared memory size
                                   0,  // stream handle
                                   kernelParams, NULL);

    cuCtxSynchronize();

    auto arr_out = (int64_t *)malloc(sizeof(int64_t) * len);
    cuMemcpyDtoH(arr_out, d_arr_out, sizeof(int64_t) * len);
    cuMemFree(d_arr);
    cout << "Output from " << kernel_name << " kernel:" << endl;
    for (int i = 0; i < len; i++) { cout << arr_out[i] << ", "; }
    cout << endl << endl;

    cuMemFree(d_arr);
    cuMemFree(d_arr_out);
}

void test_kernel() {
    /* Test kernel generation and execution*/
    auto loop = basic_transform_kernel();
    cout << "Loop IR: " << endl << loop->str() << endl;
    auto fn = CanonPass().eval(loop);

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("foo", jit->GetCtx());
    LLVMGen(*llmod).eval(fn);
    jit->Optimize(*llmod);

    auto cuda_engine = CudaEngine::Get();
    auto cuda_module = cuda_engine->Build(*llmod);
    auto kernel = cuda_engine->Lookup(cuda_module, llmod->getName().str());

    int len = 1024;
    int64_t* in_array = new int64_t[len];
    for (int i = 0; i < len; i++) {
        in_array[i] = i;
    }
    execute_kernel(llmod->getName().str(), kernel, (int64_t*)(in_array), len);
    cuda_engine->Cleanup(cuda_module);

    return;
}
#endif

shared_ptr<Func> transform_op(shared_ptr<ArrowTable2> tbl)
{
    auto t_sym = _sym("t", _i64_t);
    auto vec_in_sym = _sym("vec_in", tbl->get_data_type(1));
    auto elem_expr = vec_in_sym[{t_sym}];
    auto elem = _sym("elem", elem_expr);
    auto ten = _sym("ten", _i64_t);
    auto out_expr = _add(elem[0], ten);
    auto out = _sym("out", out_expr);
    auto op = _op(vector<Sym>{t_sym}, (_in(t_sym, vec_in_sym) & _gt(t_sym, ten)) & _lt(t_sym, _i64(64)), vector<Expr>{ out });
    auto op_sym = _sym("op", op);

    auto foo_fn = _func("foo", op_sym, vector<Sym>{vec_in_sym});
    foo_fn->tbl[elem] = elem_expr;
    foo_fn->tbl[out] = out_expr;
    foo_fn->tbl[op_sym] = op;
    foo_fn->tbl[ten] = _i64(10);

    return foo_fn;
}

shared_ptr<Func> nested_transform_op()
{
    auto a_sym = _sym("a", _i64_t);
    auto b_sym = _sym("b", _i64_t);

    auto op = _op(
        vector<Sym>{a_sym, b_sym},
        (_gte(a_sym, _i64(0)) & _lt(a_sym, _i64(10)) & _gte(b_sym, _i64(0)) & _lt(b_sym, _i64(10))),
        vector<Expr>{ a_sym + b_sym }
    );

    auto op_sym = _sym("op", op);

    auto foo_fn = _func("foo", op_sym, vector<Sym>{});
    foo_fn->tbl[op_sym] = op;

    return foo_fn;
}

int main()
{   
    /*
    test_kernel();
    return 0;
    */

    const rlim_t kStackSize = 1 * 1024 * 1024 * 1024u;   // min stack size = 2 GB
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0) {
        if (rl.rlim_cur < kStackSize) {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0) {
                cerr << "setrlimit returned result = " << result << endl;
            }
        }
    }
    auto tbl = load_arrow_file("../students.arrow").ValueOrDie();
    auto op = nested_transform_op();
    auto query_fn = compile_op<void (*)(void*, void*)>(op);

    auto status = query_arrow_file2(tbl, query_fn);

    if (!status.ok()) {
        cerr << status.ToString() << endl;
    }
    return 0;
}
