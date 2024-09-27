#include <iostream>
#include <memory>

#include <arrow/api.h>
#include <arrow/csv/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/c/bridge.h>

#include "reffine/ir/node.h"
#include "reffine/ir/expr.h"
#include "reffine/ir/loop.h"
#include "reffine/base/type.h"
#include "reffine/pass/printer.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/engine/engine.h"
#include "reffine/arrow/defs.h"

using namespace reffine;
using namespace std;

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

arrow::Status query_arrow_file(void* (*query_fn)(void*, void*))
{
    ARROW_ASSIGN_OR_RAISE(auto infile, arrow::io::ReadableFile::Open(
                "../students.arrow", arrow::default_memory_pool()));

    ARROW_ASSIGN_OR_RAISE(auto ipc_reader, arrow::ipc::RecordBatchFileReader::Open(infile));

    ARROW_ASSIGN_OR_RAISE(auto rbatch, ipc_reader->ReadRecordBatch(0));

    cout << "Input: " << endl << rbatch->ToString() << endl;

    ArrowSchema in_schema;
    ArrowArray in_array;
    ARROW_RETURN_NOT_OK(arrow::ExportRecordBatch(*rbatch, &in_array, &in_schema));

    StructSchema out_schema("output");
    StructArray out_array(in_array.length);
    out_schema.add_child<Int64Schema>("id");
    out_schema.add_child<Int64Schema>("minutes_studied");
    out_array.add_child<Int64Array>(in_array.length);
    out_array.add_child<Int64Array>(in_array.length);

    query_fn(&in_array, &out_array);

    ARROW_ASSIGN_OR_RAISE(auto res, arrow::ImportRecordBatch(&out_array, &out_schema));
    cout << "Output: " << endl << res->ToString() << endl;

    return arrow::Status::OK();
}

shared_ptr<Func> simple_fn()
{
    auto n_sym = make_shared<SymNode>("n", types::INT32);

    auto idx_alloc = make_shared<Alloc>(types::INT32);
    auto idx_addr = make_shared<SymNode>("idx_addr", idx_alloc);
    auto sum_alloc = make_shared<Alloc>(types::INT32);
    auto sum_addr = make_shared<SymNode>("sum_addr", sum_alloc);

    auto zero = make_shared<Const>(BaseType::INT32, 0);
    auto one = make_shared<Const>(BaseType::INT32, 1);
    auto two = make_shared<Const>(BaseType::INT32, 2);

    auto loop = make_shared<Loop>(make_shared<Load>(sum_addr));
    auto loop_sym = make_shared<SymNode>("loop", loop);
    loop->init = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(idx_addr, zero),
        make_shared<Store>(sum_addr, one),
    });
    loop->incr = nullptr;
    loop->exit_cond = make_shared<GreaterThanEqual>(make_shared<Load>(idx_addr), n_sym);
    loop->body_cond = nullptr;
    loop->body = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(idx_addr, make_shared<Add>(make_shared<Load>(idx_addr), one)),
        make_shared<Store>(sum_addr, make_shared<Mul>(make_shared<Load>(sum_addr), two)),
    });

    auto foo_fn = make_shared<Func>("foo", loop_sym, vector<Sym>{n_sym});
    foo_fn->tbl[loop_sym] = loop;
    foo_fn->tbl[idx_addr] = idx_alloc;
    foo_fn->tbl[sum_addr] = sum_alloc;

    return foo_fn;
}

shared_ptr<Func> abs_fn()
{
    auto a_sym = make_shared<SymNode>("a", types::INT32);
    auto b_sym = make_shared<SymNode>("b", types::INT32);

    auto cond = make_shared<GreaterThan>(a_sym, b_sym);
    auto true_body = make_shared<Sub>(a_sym, b_sym);
    auto false_body = make_shared<Sub>(b_sym, a_sym);
    auto ifelse = make_shared<IfElse>(cond, true_body, false_body);
    auto ifelse_sym = make_shared<SymNode>("abs", ifelse);

    auto foo_fn = make_shared<Func>("foo", ifelse_sym, vector<Sym>{a_sym, b_sym});
    foo_fn->tbl[ifelse_sym] = ifelse;

    return foo_fn;
}

shared_ptr<Func> vector_fn()
{
    auto vec_sym = make_shared<SymNode>("vec", types::VECTOR<1>(vector<DataType>{
        types::INT64, types::INT64, types::INT64, types::INT64, types::INT64, types::INT8, types::INT64 }));

    auto len = make_shared<Call>("get_vector_len", types::INT64, vector<Expr>{vec_sym});
    auto len_sym = make_shared<SymNode>("len", len);

    auto zero = make_shared<Const>(BaseType::INT64, 0);
    auto one = make_shared<Const>(BaseType::INT64, 1);

    auto idx_alloc = make_shared<Alloc>(types::INT64);
    auto idx_addr = make_shared<SymNode>("idx_addr", idx_alloc);
    auto sum_alloc = make_shared<Alloc>(types::INT64);
    auto sum_addr = make_shared<SymNode>("sum_addr", sum_alloc);

    auto val = make_shared<Call>("read_val", types::INT64, vector<Expr>{vec_sym, make_shared<Load>(idx_addr)});

    auto loop = make_shared<Loop>(make_shared<Load>(sum_addr));
    auto loop_sym = make_shared<SymNode>("loop", loop);
    loop->init = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(idx_addr, zero),
        make_shared<Store>(sum_addr, zero),
    });
    loop->incr = nullptr;
    loop->exit_cond = make_shared<GreaterThanEqual>(make_shared<Load>(idx_addr), len_sym);
    loop->body_cond = nullptr;
    loop->body = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(sum_addr, make_shared<Add>(make_shared<Load>(sum_addr), val)),
        make_shared<Store>(idx_addr, make_shared<Add>(make_shared<Load>(idx_addr), one)),
    });

    auto foo_fn = make_shared<Func>("foo", loop_sym, vector<Sym>{vec_sym});
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
        types::INT64, types::INT64 }));

    auto len = make_shared<Call>("get_vector_len", types::INT64, vector<Expr>{vec_in_sym});
    auto len_sym = make_shared<SymNode>("len", len);

    auto zero = make_shared<Const>(BaseType::INT64, 0);
    auto one = make_shared<Const>(BaseType::INT64, 1);

    auto idx_alloc = make_shared<Alloc>(types::INT64);
    auto idx_addr = make_shared<SymNode>("idx_addr", idx_alloc);

    auto transform = make_shared<Call>("transform_val", types::INT64, vector<Expr>{
        vec_in_sym, vec_out_sym, make_shared<Load>(idx_addr)
    });

    auto loop = make_shared<Loop>(vec_out_sym);
    auto loop_sym = make_shared<SymNode>("loop", loop);
    loop->init = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(idx_addr, zero),
    });
    loop->incr = nullptr;
    loop->exit_cond = make_shared<GreaterThanEqual>(make_shared<Load>(idx_addr), len_sym);
    loop->body_cond = nullptr;
    loop->body = make_shared<Stmts>(vector<Stmt>{
        transform,
        make_shared<Store>(idx_addr, make_shared<Add>(make_shared<Load>(idx_addr), one)),
    });

    auto foo_fn = make_shared<Func>("foo", loop_sym, vector<Sym>{vec_in_sym, vec_out_sym});
    foo_fn->tbl[len_sym] = len;
    foo_fn->tbl[idx_addr] = idx_alloc;
    foo_fn->tbl[loop_sym] = loop;

    return foo_fn;
}

int main()
{
    auto fn = transform_fn();
    cout << IRPrinter::Build(fn) << endl;

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("test", jit->GetCtx());
    LLVMGen::Build(fn, *llmod);
    cout << IRPrinter::Build(*llmod) << endl;

    jit->AddModule(std::move(llmod));
    auto query_fn = jit->Lookup<void* (*)(void*, void*)>(fn->name);

    //auto status = csv_to_arrow();
    auto status = query_arrow_file(query_fn);
    if (!status.ok()) {
        cerr << status.ToString() << endl;
    }

    return 0;
}
