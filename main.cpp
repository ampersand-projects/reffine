#include <iostream>
#include <memory>
#include <fstream>

#include <arrow/api.h>
#include <arrow/csv/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/c/bridge.h>

#include <z3++.h>
#include <cuda.h>
#include <cuda_runtime.h>

#include "reffine/ir/node.h"
#include "reffine/ir/stmt.h"
#include "reffine/ir/expr.h"
#include "reffine/ir/loop.h"
#include "reffine/ir/op.h"
#include "reffine/base/type.h"
#include "reffine/pass/printer.h"
#include "reffine/pass/canonpass.h"
#include "reffine/pass/reffinepass.h"
#include "reffine/pass/loopgen.h"
#include "reffine/pass/z3solver.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/engine/engine.h"
#include "reffine/arrow/defs.h"
#include "reffine/builder/reffiner.h"

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

arrow::Result<ArrowTable> load_arrow_file(string filename)
{
    ARROW_ASSIGN_OR_RAISE(auto file, arrow::io::ReadableFile::Open(
                filename, arrow::default_memory_pool()));

    ARROW_ASSIGN_OR_RAISE(auto ipc_reader, arrow::ipc::RecordBatchFileReader::Open(file));

    ARROW_ASSIGN_OR_RAISE(auto rbatch, ipc_reader->ReadRecordBatch(0));

    ArrowSchema schema;
    ArrowArray array;
    ARROW_RETURN_NOT_OK(arrow::ExportRecordBatch(*rbatch, &array, &schema));

    return ArrowTable(std::move(schema), std::move(array));
}

arrow::Status query_arrow_file(ArrowTable& in_table, long (*query_fn)(void*))
{
    auto& in_array = in_table.array;

    VectorSchema out_schema("output");
    VectorArray out_array(in_array.length);
    out_schema.add_child<Int64Schema>("id");
    out_schema.add_child<Int64Schema>("minutes_studied");
    out_schema.add_child<BooleanSchema>("slept_enough");
    out_array.add_child<Int64Array>(in_array.length);
    out_array.add_child<Int64Array>(in_array.length);
    out_array.add_child<BooleanArray>(in_array.length);

    cout << "SUM: " << query_fn(&in_array) << endl;

    ARROW_ASSIGN_OR_RAISE(auto res, arrow::ImportRecordBatch(&out_array, &out_schema));
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

shared_ptr<Func> basic_aggregate_kernel()
{
    /* kernel version of vector_fn */
    auto sum_out_sym = _sym("res", types::INT64.ptr());
    auto vec_in_sym = _sym("input", types::INT64.ptr());

    auto len = _idx(1024);
    auto idx_alloc = _alloc(_idx_t);
    auto idx_addr = _sym("idx_addr", idx_alloc);
    auto idx = _load(idx_addr);

    auto temp_alloc = _alloc(_i64_t);
    auto temp_addr = _sym("temp_addr", temp_alloc);
    auto temp_sum = _load(temp_addr);

    auto val_ptr = _call("get_elem_ptr", types::INT64.ptr(), vector<Expr>{vec_in_sym, idx});
    auto val = _load(val_ptr);

    auto idx_start = get_start_idx(_tidx(), _bidx(), _bdim(), _gdim(), len);
    auto idx_end = get_end_idx(_tidx(), _bidx(), _bdim(), _gdim(), len);

    auto loop = _loop(_load(sum_out_sym));

    loop->init = _stmts(vector<Stmt>{
        idx_alloc,
        _store(idx_addr, idx_start),
        temp_alloc,
        _store(temp_addr, _i64(0)),
    });
    loop->body = _stmts(vector<Stmt>{
        _store(temp_addr, _add(temp_sum, val)),
        _store(idx_addr, idx + _idx(1)),
    });
    loop->exit_cond = _gte(idx, idx_end);
    loop->post = _atomic_add(sum_out_sym, temp_sum);
    auto loop_sym = _sym("loop", loop);

    auto foo_fn = make_shared<Func>("foo", loop, vector<Sym>{sum_out_sym, vec_in_sym}, SymTable(), true);
    foo_fn->tbl[idx_addr] = idx_alloc;
    foo_fn->tbl[temp_addr] = temp_alloc;

    return foo_fn;
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

shared_ptr<Func> reduce_op_fn()
{
    auto t_sym = _sym("t", _i64_t);
    auto vec_in_sym = _sym("vec_in", _vec_t<1, int64_t, int64_t, int64_t, int64_t, int64_t, int8_t, int64_t>());
    Op op(
        { t_sym },
        ~(vec_in_sym[{t_sym}]) && _lte(t_sym, _i64(20)) &&  _gte(t_sym, _i64(10)),
        { vec_in_sym[{t_sym}][1] }
    );

    auto sum = _red(
        op,
        [] () { return _i64(0); },
        [] (Expr s, Expr v) {
            auto e = _get(v, 0);
            return _add(s, e);
        }
    );
    auto sum_sym = _sym("sum", sum);

    auto foo_fn = _func("foo", sum_sym, vector<Sym>{vec_in_sym});
    foo_fn->tbl[sum_sym] = sum;

    return foo_fn;
}

shared_ptr<Func> tpcds_query9(ArrowTable& table)
{
    auto vec_in_sym = _sym("vec_in", table.get_data_type(2));
    /*
    auto idx = _sym("idx", _idx_t);
    Op op(
        {idx},
        ~(vec_in_sym[{idx}]),
        { vec_in_sym[{idx}][10] }
    );
    auto sum = _red(
        op,
        []() { return _i64(0); },
        [](Expr s, Expr v) {
            auto e = _get(v, 0);
            return _add(s, e);
        }
    );
    auto sum_sym = _sym("sum", sum);
    */

    auto foo_fn = _func("foo", vec_in_sym, vector<Sym>{vec_in_sym});
    //foo_fn->tbl[sum_sym] = sum;

    return foo_fn;
}

#define checkCudaErrors(err) __checkCudaErrors(err, __FILE__, __LINE__)
static void __checkCudaErrors(CUresult err, const char *filename, int line)
{
    assert(filename);
    if (CUDA_SUCCESS != err) {
        const char *ename = NULL;
        const CUresult res = cuGetErrorName(err, &ename);
        fprintf(stderr,
                "CUDA API Error %04d: \"%s\" from file <%s>, "
                "line %i.\n",
                err, ((CUDA_SUCCESS == res) ? ename : "Unknown"), filename,
                line);
        exit(err);
    }
}

void execute_ptx(string kernel_name, string ptx_str, void *arg, int len)
{
    CUdevice device;
    CUmodule cudaModule;
    CUcontext context;
    CUfunction function;

    checkCudaErrors(cuInit(0));
    checkCudaErrors(cuDeviceGet(&device, 0));
    checkCudaErrors(cuCtxCreate(&context, 0, device));

    char name[128];
    cuDeviceGetName(name, 128, device);
    std::cout << "Device name: " << name << endl;

    CUdeviceptr d_arr;
    checkCudaErrors(cuMemAlloc(&d_arr, sizeof(int64_t) * len));
    checkCudaErrors(cuMemcpyHtoD(d_arr, arg, sizeof(int64_t) * len));

    CUdeviceptr d_arr_out;
    checkCudaErrors(cuMemAlloc(&d_arr_out, sizeof(int64_t) * len));

    checkCudaErrors(cuModuleLoadData(&cudaModule, ptx_str.c_str()));
    checkCudaErrors(
        cuModuleGetFunction(&function, cudaModule, kernel_name.c_str()));
    cout << "About to run " << kernel_name << " kernel..." << endl;

    int blockDimX = 32;  // num of threads per block
    int gridDimX = (len + blockDimX - 1) / blockDimX;  // num of blocks
    void *kernelParams[] = {
        &d_arr_out,
        &d_arr,
    };

    checkCudaErrors(cuLaunchKernel(function, gridDimX, 1, 1, blockDimX, 1, 1,
                                   0,  // shared memory size
                                   0,  // stream handle
                                   kernelParams, NULL));

    checkCudaErrors(cuCtxSynchronize());

    auto arr_out = (int64_t *)malloc(sizeof(int64_t) * len);
    checkCudaErrors(cuMemcpyDtoH(arr_out, d_arr_out, sizeof(int64_t) * len));
    cuMemFree(d_arr);
    cout << "Output from " << kernel_name << " kernel:" << endl;
    for (int i = 0; i < len; i++) { cout << arr_out[i] << ", "; }
    cout << endl << endl;

    cuMemFree(d_arr);
    cuMemFree(d_arr_out);
    cuModuleUnload(cudaModule);
    cuCtxDestroy(context);
}

void test_kernel() {
    /* Test kernel generation and execution*/
    // auto fn = basic_transform_kernel();
    auto fn = basic_aggregate_kernel();
    cout << "Loop IR: " << endl << IRPrinter::Build(fn) << endl;
    CanonPass::Build(fn);

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("foo", jit->GetCtx());
    LLVMGen::Build(fn, *llmod);
    jit->Optimize(*llmod);
    cout << "LLVM IR: " << endl << IRPrinter::Build(*llmod) << endl;

    auto output_ptx = LLVMGen::BuildPTX(fn, *llmod);
    cout << "Generated PTX:" << endl << output_ptx << endl;

    int len = 1024;
    int64_t* in_array = new int64_t[len];
    for (int i = 0; i < len; i++) {
        in_array[i] = i;
    }
    execute_ptx(llmod->getName().str(), output_ptx, (int64_t*)(in_array), len);

    return;
}

int main()
{
    /*
    test_kernel();
    return 0;
    */

    auto table = load_arrow_file("../benchmark/store_sales.arrow");

    auto fn = tpcds_query9(*table);
    cout << "Reffine IR:" << endl << IRPrinter::Build(fn) << endl;
    return 0;
    auto fn2 = OpToLoop::Build(fn);
    cout << "OpToLoop IR: " << endl << IRPrinter::Build(fn2) << endl;

    auto loop = LoopGen::Build(fn2);
    cout << "Loop IR:" << endl << IRPrinter::Build(loop) << endl;
    CanonPass::Build(loop);

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("test", jit->GetCtx());
    LLVMGen::Build(loop, *llmod);
    
    jit->Optimize(*llmod);

    // dump llvm IR to .ll file
    ofstream llfile(llmod->getName().str() + ".ll");
    llfile << IRPrinter::Build(*llmod);
    llfile.close();

    if (llvm::verifyModule(*llmod)) {
        throw std::runtime_error("LLVM module verification failed!!!");
    }

    jit->AddModule(std::move(llmod));
    auto query_fn = jit->Lookup<long (*)(void*)>(fn->name);

    //auto status = csv_to_arrow();
    auto status = query_arrow_file(*table, query_fn);
    if (!status.ok()) {
        cerr << status.ToString() << endl;
    }

    return 0;
}
