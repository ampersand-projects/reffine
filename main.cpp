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

shared_ptr<Func> vector_fn()
{
    auto vec_sym = _sym("vec", types::VECTOR<1>(vector<DataType>{
        _i64_t, _i64_t, _i64_t, _i64_t, _i64_t, _i8_t, _i64_t }));

    auto len = _call("get_vector_len", _idx_t, vector<Expr>{vec_sym});
    auto len_sym = _sym("len", len);

    auto kernel = make_shared<GetKernelInfo>(_i64(1), _i64(1), _i64(1));
    auto kernel_sym = _sym("kernel", kernel);

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
    foo_fn->tbl[kernel_sym] = kernel;

    return foo_fn;
}


int main()
{
    auto fn = vector_fn();
    cout << "Reffine IR: " << endl << IRPrinter::Build(fn) << endl;
    // auto fn2 = OpToLoop::Build(fn);
    // cout << "OpToLoop IR: " << endl << IRPrinter::Build(fn2) << endl;
    // auto loop = LoopGen::Build(fn);
    // cout << "Loop IR:" << endl << IRPrinter::Build(loop) << endl;
    return 0;
    CanonPass::Build(fn);
    cout << "Canon IR:" << endl << IRPrinter::Build(fn) << endl;

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("test", jit->GetCtx());
    LLVMGen::Build(fn, *llmod);

    cout << "LLVM IR:" << endl << IRPrinter::Build(*llmod) << endl;
    
    // jit->Optimize(*llmod);
    // cout << "Optimized LLVM IR:" << endl << IRPrinter::Build(*llmod) << endl;

    jit->GeneratePTX(*llmod);
    cout << "Generated PTX:" << endl << IRPrinter::Build(*llmod) << endl;

    std::string output_ptx;
    jit->GeneratePTX(*llmod, output_ptx);
    cout << "Generated PTX 2:" << endl << output_ptx << endl;

    ArrowArray in_array;
    auto status = get_arrow_array(in_array);
    if (!status.ok()) {
        cerr << status.ToString() << endl;
    }

    jit->ExecutePTX(output_ptx, llmod->getName().str(), (void*) (&in_array));

    // dump llvm IR to .ll file
    ofstream llfile(llmod->getName().str() + ".ll");
    llfile << IRPrinter::Build(*llmod);
    llfile.close();

    if (llvm::verifyModule(*llmod)) {
        throw std::runtime_error("LLVM module verification failed!!!");
    }

    jit->AddModule(std::move(llmod));
    // auto query_fn = jit->Lookup<int (*)(void*)>(fn->name);

    // //auto status = csv_to_arrow();
    // auto status2 = run_vector_fn(query_fn);
    // if (!status2.ok()) {
    //     cerr << status.ToString() << endl;
    // }

    return 0;
}
