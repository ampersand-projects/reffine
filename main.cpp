#include <iostream>
#include <memory>
#include <fstream>
#include <iomanip>
#include <sys/resource.h>

#include <arrow/api.h>
#include <arrow/csv/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/c/bridge.h>

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

using namespace reffine;
using namespace std;
using namespace reffine::reffiner;

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


arrow::Status query_arrow_file(ArrowTable& in_table, void (*query_fn)(long*, void*))
{
    auto& in_array = in_table.array;

    long sum;
    query_fn(&sum, &in_array);
    cout << "SUM: " << sum << endl;

    return arrow::Status::OK();
}

arrow::Status query_arrow_file2(shared_ptr<ArrowTable> in_table, void (*query_fn)(void*, void*))
{
    ArrowTable* out_table;

    query_fn(&out_table, in_table.get());

    ARROW_ASSIGN_OR_RAISE(auto res, arrow::ImportRecordBatch(out_table->array, out_table->schema));
    cout << "Output: " << endl << res->ToString() << endl;

    return arrow::Status::OK();
}

shared_ptr<Func> vector_fn()
{
    auto vec_sym = _sym("vec", types::VECTOR<1>(vector<DataType>{
        _i64_t, _i64_t, _i64_t, _i64_t, _i64_t, _i8_t, _i64_t }));

    auto len = _call("get_vector_len", _idx_t, vector<Expr>{vec_sym});
    auto len_sym = _sym("len", len);

    auto idx_alloc = _alloc(_idx_t);
    auto idx_addr = _sym("idx_addr", idx_alloc);
    auto idx = _load(idx_addr);
    auto sum_alloc = _alloc(_i64_t);
    auto sum_addr = _sym("sum_addr", sum_alloc);
    auto sum = _load(sum_addr);

    auto val_ptr = _fetch(vec_sym, idx, 1);
    auto val = _load(val_ptr);

    auto loop = _loop(_load(sum_addr));
    auto loop_sym = _sym("loop", loop);
    loop->init = _stmts(vector<Stmt>{
        _store(idx_addr, _idx(0)),
        _store(sum_addr, _i64(0)),
    });
    loop->exit_cond = _gte(idx, len_sym);
    loop->body = _stmts(vector<Stmt>{
        _store(sum_addr, sum + val),
        _store(idx_addr, idx + _idx(1)),
    });

    auto foo_fn = _func("foo", loop_sym, vector<Sym>{vec_sym});
    foo_fn->tbl[len_sym] = len;
    foo_fn->tbl[idx_addr] = idx_alloc;
    foo_fn->tbl[sum_addr] = sum_alloc;
    foo_fn->tbl[loop_sym] = loop;

    return foo_fn;
}

shared_ptr<Func> transform_fn()
{
    auto vec_in_sym = make_shared<SymNode>("vec_in", types::VECTOR<1>(vector<DataType>{
        types::INT64, types::INT64, types::INT64, types::INT64, types::INT64, types::INT8, types::INT64 }));
    auto vec_out_sym = make_shared<SymNode>("vec_out", types::VECTOR<1>(vector<DataType>{
        types::INT64, types::INT64, types::INT8 }));

    auto len = make_shared<Call>("get_vector_len", types::IDX, vector<Expr>{vec_in_sym});
    auto len_sym = make_shared<SymNode>("len", len);

    auto zero = make_shared<Const>(types::IDX, 0);
    auto one = make_shared<Const>(types::IDX, 1);
    auto eight = make_shared<Const>(types::INT64, 8);
    auto sixty = make_shared<Const>(types::INT64, 60);
    auto twenty = make_shared<Const>(types::INT64, 20);
    auto _true = make_shared<Const>(types::BOOL, 1);
    auto _false = make_shared<Const>(types::BOOL, 0);

    auto idx_alloc = make_shared<Alloc>(types::IDX);
    auto idx_addr = make_shared<SymNode>("idx_addr", idx_alloc);
    auto idx = make_shared<Load>(idx_addr);

    auto id_valid = make_shared<IsValid>(vec_in_sym, idx, 0);
    auto id_data_ptr = make_shared<FetchDataPtr>(vec_in_sym, idx, 0);
    auto id_data = make_shared<Load>(id_data_ptr);
    auto hours_valid = make_shared<IsValid>(vec_in_sym, idx, 1);
    auto hours_data_ptr = make_shared<FetchDataPtr>(vec_in_sym, idx, 1);
    auto hours_data = make_shared<Load>(hours_data_ptr);
    auto hours_slept_data_ptr = make_shared<FetchDataPtr>(vec_in_sym, idx, 3);
    auto hours_slept_data = make_shared<Load>(hours_slept_data_ptr);

    auto out_id_data_ptr = make_shared<FetchDataPtr>(vec_out_sym, idx, 0);
    auto out_minutes_data_ptr = make_shared<FetchDataPtr>(vec_out_sym, idx, 1);
    auto out_sleep_data_ptr = make_shared<FetchDataPtr>(vec_out_sym, idx, 2);
    auto out_minutes = make_shared<Mul>(hours_data, sixty);

    auto loop = _loop(vec_out_sym);
    auto loop_sym = make_shared<SymNode>("loop", loop);
    loop->init = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(idx_addr, zero),
    });
    loop->incr = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(idx_addr, make_shared<Add>(make_shared<Load>(idx_addr), one)),
    });
    loop->exit_cond = make_shared<GreaterThanEqual>(make_shared<Load>(idx_addr), len_sym);
    loop->body = make_shared<Stmts>(vector<Stmt>{
        make_shared<IfElse>(
            make_shared<And>(
                make_shared<And>(id_valid, hours_valid),
                make_shared<GreaterThanEqual>(hours_data, twenty)
            ),
            make_shared<Stmts>(vector<Stmt>{
                make_shared<Store>(out_id_data_ptr, make_shared<Load>(id_data_ptr)),
                make_shared<Store>(out_minutes_data_ptr, out_minutes),
                make_shared<SetValid>(vec_out_sym, idx, _true, 0),
                make_shared<SetValid>(vec_out_sym, idx, _true, 1),
            }),
            make_shared<Stmts>(vector<Stmt>{
                make_shared<SetValid>(vec_out_sym, idx, _false, 0),
                make_shared<SetValid>(vec_out_sym, idx, _false, 1),
            })
        ),
        make_shared<Store>(
            out_sleep_data_ptr,
            make_shared<Select>(
                make_shared<LessThan>(hours_slept_data, eight),
		make_shared<Const>(types::INT8, 0),
		make_shared<Const>(types::INT8, 1)
	    )
        ),
        make_shared<SetValid>(vec_out_sym, idx, _true, 2)
    });
    loop->post = make_shared<Call>("set_vector_len", types::INT64, vector<Expr>{
        vec_out_sym, make_shared<Load>(idx_addr)
    });

    auto foo_fn = make_shared<Func>("foo", loop_sym, vector<Sym>{vec_in_sym, vec_out_sym});
    foo_fn->tbl[len_sym] = len;
    foo_fn->tbl[idx_addr] = idx_alloc;
    foo_fn->tbl[loop_sym] = loop;

    return foo_fn;
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

    loop->init = _stmts(vector<Stmt>{
        idx_alloc,
        _store(idx_addr, idx_start),
    });
    loop->body = _stmts(vector<Stmt>{
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

void demorgan_test()
{
    auto t = make_shared<SymNode>("t", types::INT64);
    auto lb = make_shared<Const>(types::INT64, 10);
    auto pred = make_shared<GreaterThanEqual>(t, lb);

    auto p = make_shared<SymNode>("p", t);
    auto forall = make_shared<ForAll>(t,
        make_shared<And>(
            make_shared<Implies>(make_shared<GreaterThanEqual>(t, p), pred),
            make_shared<Implies>(make_shared<LessThan>(t, p), make_shared<Not>(pred))
        )
    );

    Z3Solver z3s;
    int res = z3s.solve(forall, p).as_int64();
    cout << "p = " << res << endl;
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
    auto fn = basic_transform_kernel();
    cout << "Loop IR: " << endl << IRPrinter::Build(fn) << endl;
    CanonPass::Build(fn);

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("foo", jit->GetCtx());
    LLVMGen::Build(fn, *llmod);
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
    auto out_expr = _add(elem[0], _i64(10));
    auto out = _sym("out", out_expr);
    auto op = _op(vector<Sym>{t_sym}, ~(elem) & _gt(t_sym, _i64(5)) , vector<Expr>{ out });
    auto op_sym = _sym("op", op);

    auto foo_fn = _func("foo", op_sym, vector<Sym>{vec_in_sym});
    foo_fn->tbl[elem] = elem_expr;
    foo_fn->tbl[out] = out_expr;
    foo_fn->tbl[op_sym] = op;

    return foo_fn;
}

shared_ptr<Func> vector_op()
{
    auto t_sym = _sym("t", _i64_t);
    auto vec_in_sym =
        _sym("vec_in", _vec_t<1, int64_t, int64_t, int64_t, int64_t, int64_t,
                              int8_t, int64_t>());
    Op op(
        {t_sym},
        ~(vec_in_sym[{t_sym}]) & _lte(t_sym, _i64(48)) & _gte(t_sym, _i64(10)),
        {
            vec_in_sym[{t_sym}][2],
            _new(vector<Expr>{
                vec_in_sym[{t_sym}][1],
                vec_in_sym[{t_sym}][2],
                vec_in_sym[{t_sym}][0],
                vec_in_sym[{t_sym}][3],
            }),
            vec_in_sym[{t_sym}][3],
        });

    auto sum = _red(
        op,
        []() {
            return _new(vector<Expr>{
                _new(vector<Expr>{_i64(0), _i64(0)}),
                _new(vector<Expr>{_i64(0), _i64(0)}),
            });
        },
        [](Expr s, Expr v) {
            auto v0 = _get(_get(v, 2), 0);
            auto v1 = _get(_get(v, 2), 1);
            auto v2 = _get(_get(v, 2), 2);
            auto v3 = _get(_get(v, 2), 3);
            auto s0 = _get(_get(s, 0), 0);
            auto s1 = _get(_get(s, 0), 1);
            auto s2 = _get(_get(s, 1), 0);
            auto s3 = _get(_get(s, 1), 1);
            return _new(vector<Expr>{
                _new(vector<Expr>{_add(s0, v0), _add(s1, v1)}),
                _new(vector<Expr>{_add(s2, v2), _add(s3, v3)}),
            });
        });
    auto sum_sym = _sym("sum", sum);
    auto res = _get(_get(sum_sym, 1), 0);
    auto res_sym = _sym("res", res);

    auto foo_fn = _func("foo", res_sym, vector<Sym>{vec_in_sym});
    foo_fn->tbl[sum_sym] = sum;
    foo_fn->tbl[res_sym] = res;

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
    auto op = vector_op();
    auto query_fn = compile_op<void (*)(void*, void*)>(op);

    auto status = query_arrow_file2(tbl, query_fn);

    if (!status.ok()) {
        cerr << status.ToString() << endl;
    }
    return 0;
}
