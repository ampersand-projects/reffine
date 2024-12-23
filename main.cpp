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
#include "reffine/pass/loopgen.h"
#include "reffine/pass/z3solver.h"
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

arrow::Status query_arrow_file(long (*query_fn)(void*))
{
    ARROW_ASSIGN_OR_RAISE(auto infile, arrow::io::ReadableFile::Open(
                "../students.arrow", arrow::default_memory_pool()));

    ARROW_ASSIGN_OR_RAISE(auto ipc_reader, arrow::ipc::RecordBatchFileReader::Open(infile));

    ARROW_ASSIGN_OR_RAISE(auto rbatch, ipc_reader->ReadRecordBatch(0));

    cout << "Input: " << endl << rbatch->ToString() << endl;

    ArrowSchema in_schema;
    ArrowArray in_array;
    ARROW_RETURN_NOT_OK(arrow::ExportRecordBatch(*rbatch, &in_array, &in_schema));

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
    auto vec_sym = make_shared<SymNode>("vec", types::VECTOR<1>(vector<DataType>{
        types::INT64, types::INT64, types::INT64, types::INT64, types::INT64, types::INT8, types::INT64 }));

    auto len = make_shared<Call>("get_vector_len", types::IDX, vector<Expr>{vec_sym});
    auto len_sym = make_shared<SymNode>("len", len);

    auto idx_alloc = make_shared<Alloc>(types::IDX);
    auto idx_addr = make_shared<SymNode>("idx_addr", idx_alloc);
    auto idx = make_shared<Load>(idx_addr);
    auto sum_alloc = make_shared<Alloc>(types::INT64);
    auto sum_addr = make_shared<SymNode>("sum_addr", sum_alloc);
    auto sum = make_shared<Load>(sum_addr);

    auto val_ptr = make_shared<FetchDataPtr>(vec_sym, idx, 1);
    auto val = make_shared<Load>(val_ptr);

    auto loop = make_shared<Loop>(make_shared<Load>(sum_addr));
    auto loop_sym = make_shared<SymNode>("loop", loop);
    loop->init = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(idx_addr, make_shared<Const>(BaseType::IDX, 0)),
        make_shared<Store>(sum_addr, make_shared<Const>(BaseType::INT64, 0)),
    });
    loop->exit_cond = make_shared<GreaterThanEqual>(idx, len_sym);
    loop->body = make_shared<Stmts>(vector<Stmt>{
        make_shared<Store>(sum_addr, make_shared<Add>(sum, val)),
        make_shared<Store>(idx_addr, make_shared<Add>(idx, make_shared<Const>(BaseType::IDX, 1))),
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
        types::INT64, types::INT64, types::INT8 }));

    auto len = make_shared<Call>("get_vector_len", types::IDX, vector<Expr>{vec_in_sym});
    auto len_sym = make_shared<SymNode>("len", len);

    auto zero = make_shared<Const>(BaseType::IDX, 0);
    auto one = make_shared<Const>(BaseType::IDX, 1);
    auto eight = make_shared<Const>(BaseType::INT64, 8);
    auto sixty = make_shared<Const>(BaseType::INT64, 60);
    auto twenty = make_shared<Const>(BaseType::INT64, 20);
    auto _true = make_shared<Const>(BaseType::BOOL, 1);
    auto _false = make_shared<Const>(BaseType::BOOL, 0);

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

    auto loop = make_shared<Loop>(vec_out_sym);
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
		make_shared<Const>(BaseType::INT8, 0),
		make_shared<Const>(BaseType::INT8, 1)
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

shared_ptr<Func> test_op_fn()
{
    auto t_sym = make_shared<SymNode>("t", types::INT64);
    Op op(
        vector<Sym>{t_sym},
        vector<Expr>{
            make_shared<GreaterThan>(t_sym, make_shared<Const>(BaseType::INT64, 0)),
            make_shared<LessThan>(t_sym, make_shared<Const>(BaseType::INT64, 10)),
        },
        vector<Expr>{t_sym}
    );

    auto sum = make_shared<Reduce>(
        op,
        [] () { return make_shared<Const>(BaseType::INT64, 0); },
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
    auto lb = make_shared<Const>(BaseType::INT64, 10);
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

int main()
{
    auto fn = test_op_fn();
    cout << "Reffine IR:" << endl << IRPrinter::Build(fn) << endl;
    auto loop = LoopGen::Build(fn);
    cout << "Loop IR (before canon):" << endl << IRPrinter::Build(loop) << endl;
    CanonPass::Build(loop);
    cout << "Loop IR (after canon):" << endl << IRPrinter::Build(loop) << endl;

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("test", jit->GetCtx());
    LLVMGen::Build(loop, *llmod);
    if (llvm::verifyModule(*llmod)) {
        throw std::runtime_error("LLVM module verification failed!!!");
    }

    // dump llvm IR to .ll file
    ofstream llfile(llmod->getName().str() + ".ll");
    llfile << IRPrinter::Build(*llmod);
    llfile.close();

    jit->AddModule(std::move(llmod));
    auto query_fn = jit->Lookup<long (*)()>(fn->name);
    cout << "Result: " << query_fn() << endl;

    //auto status = csv_to_arrow();
    /*
    auto status = query_arrow_file(query_fn);
    if (!status.ok()) {
        cerr << status.ToString() << endl;
    }
    */

    return 0;
}
