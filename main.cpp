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

#include "reffine/ir/node.h"
#include "reffine/ir/stmt.h"
#include "reffine/ir/expr.h"
#include "reffine/ir/loop.h"
#include "reffine/ir/op.h"
#include "reffine/base/type.h"
#include "reffine/pass/printer.h"
#include "reffine/pass/canonpass.h"
#include "reffine/pass/cudagen.h"
#include "reffine/pass/cudapass.h"
#include "reffine/pass/reffinepass.h"
#include "reffine/pass/loopgen.h"
#include "reffine/pass/z3solver.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/engine/engine.h"
#include "reffine/engine/cudaengine.h"
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

arrow::Status run_vector_fn(int (*query_fn)(void*))
{
    ARROW_ASSIGN_OR_RAISE(auto infile, arrow::io::ReadableFile::Open(
                "../students.arrow", arrow::default_memory_pool()));

    ARROW_ASSIGN_OR_RAISE(auto ipc_reader, arrow::ipc::RecordBatchFileReader::Open(infile));

    ARROW_ASSIGN_OR_RAISE(auto rbatch, ipc_reader->ReadRecordBatch(0));

    cout << rbatch->ToString() << endl;

    ArrowSchema in_schema;
    ArrowArray in_array;

    ARROW_RETURN_NOT_OK(arrow::ExportRecordBatch(*rbatch, &in_array, &in_schema));
    // arrow_print_schema(&in_schema);
    // arrow_print_array(&in_array);
    
    int res;
    res = query_fn(&in_array);
    cout << "Result: " << res << endl;

    return arrow::Status::OK();
}


arrow::Status get_arrow_array(ArrowArray& in_array) {
    // for CUDA testing

    ARROW_ASSIGN_OR_RAISE(auto infile, arrow::io::ReadableFile::Open(
        "../students.arrow", arrow::default_memory_pool()));

    ARROW_ASSIGN_OR_RAISE(auto ipc_reader, arrow::ipc::RecordBatchFileReader::Open(infile));

    ARROW_ASSIGN_OR_RAISE(auto rbatch, ipc_reader->ReadRecordBatch(0));

    // cout << rbatch->ToString() << endl;

    ArrowSchema in_schema;
    // ArrowArray in_array;

    ARROW_RETURN_NOT_OK(arrow::ExportRecordBatch(*rbatch, &in_array, &in_schema));

    // return in_array;
    return arrow::Status::OK();
}

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


shared_ptr<Func> vector_fn()
{
    auto vec_sym = _sym("vec", types::VECTOR<1>(vector<DataType>{
        _i64_t, _i64_t, _i64_t, _i64_t, _i64_t, _i8_t, _i64_t }));
    // auto vec_sym = make_shared<SymNode>("vec", types::INT64.ptr());

    auto len = _call("get_vector_len", _idx_t, vector<Expr>{vec_sym});
    auto len_sym = _sym("len", len);

    auto tid = make_shared<ThreadIdx>();
    auto bid = make_shared<BlockIdx>();
    auto bdim = make_shared<BlockDim>();
    auto gdim = make_shared<GridDim>();
    // auto start = make_shared<IdxStart>(tid, bid, bdim, gdim, len);
    // auto idx_start = _cast(_idx_t, start);
    // auto idx_end = make_shared<IdxEnd>(tid, bid, bdim, gdim, len);
    auto idx_start = get_start_idx(tid, bid, bdim, gdim, len);
    auto idx_end = get_end_idx(tid, bid, bdim, gdim, len);

    auto idx_alloc = _alloc(_idx_t);
    // auto idx_alloc = _alloc(_idx_t, make_shared<Const>(types::UINT64, 1));
    auto idx_addr = _sym("idx_addr", idx_alloc);
    auto idx = _load(idx_addr);
    auto sum_alloc = _alloc(_i64_t);
    auto sum_addr = _sym("sum_addr", sum_alloc);
    auto sum = _load(sum_addr);
    auto sum_sym = _sym("sum", sum);

    auto val_ptr = _fetch(vec_sym, idx, 1);
    auto val = _load(val_ptr);

    auto loop = _loop(_load(sum_addr));
    auto loop_sym = _sym("loop", loop);
    loop->init = _stmts(vector<Stmt>{
        // _store(idx_addr, _idx(0)),
        _store(idx_addr, idx_start),
        _store(sum_addr, _i64(0)),
    });
    // loop->exit_cond = _gte(idx, len_sym);
    loop->exit_cond = _gte(idx, idx_end);
    loop->body = _stmts(vector<Stmt>{
        _store(sum_addr, sum + val),
        _store(idx_addr, idx + _idx(1)),
    });

    auto foo_fn = _func("foo", loop_sym, vector<Sym>{vec_sym, sum_sym});
    foo_fn->tbl[len_sym] = len;
    foo_fn->tbl[idx_addr] = idx_alloc;
    foo_fn->tbl[sum_addr] = sum_alloc;
    foo_fn->tbl[loop_sym] = loop;
    // foo_fn->tbl[kernel_sym] = kernel;
    // foo_fn->tbl[tid_sym] = tid;

    return foo_fn;
}

shared_ptr<Func> vector_fn_2()
{
    /* Retry with regular array (rather than Arrow Array) */

    // auto vec_sym = _sym("vec", types::VECTOR<1>(vector<DataType>{
    //     _i64_t, _i64_t, _i64_t, _i64_t, _i64_t, _i8_t, _i64_t }));
    auto vec_sym = make_shared<SymNode>("vec", types::INT64.ptr());
    auto sum_sym = _sym("sum", types::INT64.ptr());

    auto len = _call("get_vector_len", _idx_t, vector<Expr>{vec_sym});
    auto len_sym = _sym("len", len);

    auto tid = make_shared<ThreadIdx>();
    auto bid = make_shared<BlockIdx>();
    auto bdim = make_shared<BlockDim>();
    auto gdim = make_shared<GridDim>();
    // auto idx_start = get_start_idx(tid, bid, bdim, gdim, len);
    // auto idx_end = get_end_idx(tid, bid, bdim, gdim, len);

    auto idx_alloc = _alloc(_idx_t);
    // auto idx_alloc = _alloc(_idx_t, make_shared<Const>(types::UINT64, 1));
    auto idx_addr = _sym("idx_addr", idx_alloc);
    auto idx = _load(idx_addr);
    auto sum_alloc = _alloc(_i64_t);
    // auto sum_addr = _sym("sum_addr", sum_alloc);
    // auto sum_addr = _call("get_elem_ptr", types::INT64.ptr(), vector<Expr>{sum_sym, _idx(0)});
    // auto sum = _load(sum_addr);
    auto sum = _load(sum_sym);
    // auto sum_sym = _sym("sum", sum);

    auto val_ptr = _call("get_elem_ptr", types::INT64.ptr(), vector<Expr>{vec_sym, idx});
    auto val = _load(val_ptr);

    auto loop = _loop(_load(sum_sym)); 
    // auto loop = _loop(_load(sum_sym)); 
    auto loop_sym = _sym("loop", loop);
    loop->init = _stmts(vector<Stmt>{
        _store(idx_addr, _idx(0)),
        // _store(idx_addr, idx_start),
        _store(sum_sym, _i64(0))
        // _store(sum_sym, _i64(0)),
    });
    // loop->exit_cond = _gte(idx, len_sym);
    loop->exit_cond = _gte(idx, _idx(1));
    // loop->exit_cond = _gte(idx, idx_end);
    loop->body = _stmts(vector<Stmt>{
        _store(sum_sym, sum + val),
        // _store(sum_addr, val),
        // _store(sum_addr, sum + _i64(5)),
        _store(idx_addr, idx + _idx(1)),
    });

    auto foo_fn = _func("foo", loop_sym, vector<Sym>{vec_sym, sum_sym});
    // auto foo_fn = _func("foo", loop_sym, vector<Sym>{vec_sym});
    foo_fn->tbl[len_sym] = len;
    foo_fn->tbl[idx_addr] = idx_alloc;
    // foo_fn->tbl[sum_addr] = sum_alloc;
    // foo_fn->tbl[sum_sym] = sum;
    foo_fn->tbl[loop_sym] = loop;

    return foo_fn;
}

shared_ptr<Func> vector_fn_3()
{
    /* full vector_fn example */
    auto res_sym = _sym("res", types::INT64.ptr());
    auto input_sym = _sym("input", types::INT64.ptr());
    auto idx_sym = _sym("idx", types::IDX.ptr());

    auto idx = _load(idx_sym);
    auto len = _idx(1024);

    auto val_ptr = _call("get_elem_ptr", types::INT64.ptr(), vector<Expr>{input_sym, idx});
    auto val = _load(val_ptr);

    auto tid = make_shared<ThreadIdx>();
    auto bid = make_shared<BlockIdx>();
    auto bdim = make_shared<BlockDim>();
    auto gdim = make_shared<GridDim>();

    auto idx_start = get_start_idx(tid, bid, bdim, gdim, len);
    auto idx_end = get_end_idx(tid, bid, bdim, gdim, len);

    auto loop = _loop(_load(res_sym));
    loop->init = _stmts(vector<Stmt>{
        _store(idx_sym, idx_start),
        _store(res_sym, _add(_load(res_sym), _load(input_sym))),
    });
    loop->body = _stmts(vector<Stmt>{
        _store(res_sym, _add(_load(res_sym), val)),
        _store(idx_sym, idx + _idx(1)),
    });
    loop->exit_cond = _gte(idx, idx_end);
    auto loop_sym = _sym("loop", loop);

    auto foo_fn = _func("foo", loop, vector<Sym>{input_sym, res_sym, idx_sym});
    foo_fn->tbl[loop_sym] = loop;

    return foo_fn;
}

shared_ptr<Func> basic_transform_fn()
{
    /* take in array and output array thats same thing + 1*/
    auto vec_out_sym = _sym("res", types::INT64.ptr());
    auto vec_in_sym = _sym("input", types::INT64.ptr());

    auto len = _idx(1024);
    auto idx_alloc = _alloc(_idx_t);
    auto idx_addr = _sym("idx_addr", idx_alloc);
    auto idx = _load(idx_addr);

    auto val_ptr = _call("get_elem_ptr", types::INT64.ptr(), vector<Expr>{vec_in_sym, idx});
    auto val = _load(val_ptr);

    auto out_ptr = _call("get_elem_ptr", types::INT64.ptr(), vector<Expr>{vec_out_sym, idx});

    auto tid = make_shared<ThreadIdx>();
    auto bid = make_shared<BlockIdx>();
    auto bdim = make_shared<BlockDim>();
    auto gdim = make_shared<GridDim>();

    auto idx_start = get_start_idx(tid, bid, bdim, gdim, len);
    auto idx_end = get_end_idx(tid, bid, bdim, gdim, len);

    auto loop = _loop(_load(vec_out_sym));

    loop->init = _stmts(vector<Stmt>{
        _store(idx_addr, idx_start),
    });
    loop->body = _stmts(vector<Stmt>{
        _store(out_ptr, _add(_i64(1), val)),
        _store(idx_addr, idx + _idx(1)),
    });
    loop->exit_cond = _gte(idx, idx_end);
    auto loop_sym = _sym("loop", loop);

    auto foo_fn = _func("foo", loop, vector<Sym>{vec_out_sym, vec_in_sym});
    foo_fn->tbl[loop_sym] = loop;
    foo_fn->tbl[idx_addr] = idx_alloc;

    return foo_fn;
}

shared_ptr<Kernel> basic_transform_kernel()
{
    /* make kernel version */
    auto vec_out_sym = _sym("res", types::INT64.ptr());
    auto vec_in_sym = _sym("input", types::INT64.ptr());

    auto len = _idx(1024);
    auto idx_alloc = _alloc(_idx_t);
    auto idx_addr = _sym("idx_addr", idx_alloc);
    auto idx = _load(idx_addr);

    auto val_ptr = _call("get_elem_ptr", types::INT64.ptr(), vector<Expr>{vec_in_sym, idx});
    auto val = _load(val_ptr);

    auto out_ptr = _call("get_elem_ptr", types::INT64.ptr(), vector<Expr>{vec_out_sym, idx});

    auto loop = _loop(_load(vec_out_sym));

    loop->init = _stmts(vector<Stmt>{
        _store(idx_addr, _idx(0)),
    });
    loop->body = _stmts(vector<Stmt>{
        _store(out_ptr, _add(_i64(1), val)),
        _store(idx_addr, idx + _idx(1)),
    });
    loop->exit_cond = _gte(idx, _idx(10));
    auto loop_sym = _sym("loop", loop);

    auto foo_fn = make_shared<Kernel>("foo", loop, vector<Sym>{vec_out_sym, vec_in_sym});
    foo_fn->tbl[loop_sym] = loop;
    foo_fn->tbl[idx_addr] = idx_alloc;

    return foo_fn;
}

int get_test_input_array(int64_t* in_array, int len) {
    int true_res = 0;
    // int64_t* in_array = new int64_t[len];
    for (int i = 0; i < len; i++) {
        in_array[i] = i;
        true_res += i;
    }

    return true_res;
}



int main()
{
    // auto fn = vector_fn();
    // auto fn = vector_fn_2();
    // auto fn = vector_fn_3();
    // auto fn = basic_transform_fn();
    auto fn = basic_transform_kernel();
    // return 0;
    cout << "Reffine IR: " << endl << IRPrinter::Build(fn) << endl;
    // return 0;
    
    // auto fn2 = OpToLoop::Build(fn);
    // cout << "OpToLoop IR: " << endl << IRPrinter::Build(fn2) << endl;
    // auto loop = LoopGen::Build(fn);
    // cout << "Loop IR:" << endl << IRPrinter::Build(loop) << endl;
    // return 0;
    // cout << "About to build Canon IR" << endl;
    // CanonPass::Build(fn);
    // cout << "Finished building Canon IR" << endl;
    cout << "Canon IR:" << endl << IRPrinter::Build(fn) << endl;

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("foo", jit->GetCtx());
    CUDAPass::Build(fn, *llmod);
    cout << "CUDAPass IR: " << endl << IRPrinter::Build(fn) << endl;
    CUDAGen::Build(fn, *llmod);
    cout << "LLVM IR:" << endl << IRPrinter::Build(*llmod) << endl;
    // return 0;
    
    // jit->Optimize(*llmod);
    // cout << "Optimized LLVM IR:" << endl << IRPrinter::Build(*llmod) << endl;

    std::string output_ptx;
    auto cuda = CUDAEngine::Init(*llmod);
    cuda->GeneratePTX();
    cuda->PrintPTX();

    int len = 1024;
    int64_t* in_array = new int64_t[len];
    int true_res = get_test_input_array(in_array, len);
    int *result;
    result = (int*)malloc(sizeof(int));
    cuda->ExecutePTX((int64_t*)(in_array), result);
    cout << "True Res: " << true_res << endl;

    return 0; 

    // dump llvm IR to .ll file
    ofstream llfile(llmod->getName().str() + ".ll");
    llfile << IRPrinter::Build(*llmod);
    llfile.close();

    if (llvm::verifyModule(*llmod)) {
        throw std::runtime_error("LLVM module verification failed!!!");
    }

    jit->AddModule(std::move(llmod));
    auto query_fn = jit->Lookup<int (*)(void*, int*)>(fn->name);

    cout << "\nAbout to run query_fn..." << endl;
    int sum;
    int res = query_fn(in_array, &sum);
    cout << "True Res: " << true_res << endl;
    cout << "Result: " << res << endl;


    // //auto status = csv_to_arrow();
    // auto status2 = run_vector_fn(query_fn);
    // if (!status2.ok()) {
    //     cerr << status.ToString() << endl;
    // }

    return 0;
}
