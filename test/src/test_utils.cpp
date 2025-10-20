#include "reffine/builder/reffiner.h"
#include "test_utils.h"

using namespace reffine;
using namespace reffine::reffiner;

arrow::Result<std::shared_ptr<ArrowTable2>> get_input_vector()
{
    ARROW_ASSIGN_OR_RAISE(
        auto infile, arrow::io::ReadableFile::Open(
                         "../../students.arrow", arrow::default_memory_pool()));
    ARROW_ASSIGN_OR_RAISE(auto ipc_reader,
                          arrow::ipc::RecordBatchFileReader::Open(infile));
    ARROW_ASSIGN_OR_RAISE(auto rbatch, ipc_reader->ReadRecordBatch(0));

    auto table = std::make_shared<ArrowTable2>();
    ARROW_RETURN_NOT_OK(
        arrow::ExportRecordBatch(*rbatch, table->array, table->schema));

    return table;
}

std::string print_arrow_table(ArrowTable* tbl)
{
    auto res = arrow::ImportRecordBatch(tbl->array, tbl->schema).ValueOrDie();
    return res->ToString();
}

static shared_ptr<Func> gen_table_op()
{
    auto t_sym = _sym("t", _i64_t);
    auto lb_sym = _sym("lb", _i64_t);
    auto ub_sym = _sym("ub", _i64_t);

    auto op = _op(
        vector<Sym>{t_sym},
        (_gte(t_sym, lb_sym) & _lt(t_sym, ub_sym)),
        vector<Expr>{ t_sym }
    );
    auto op_sym = _sym("op", op);

    auto fn = _func("gen_table", op_sym, vector<Sym>{lb_sym, ub_sym});
    fn->tbl[op_sym] = op;

    return fn;
}

gen_table_ty gen_fake_table()
{
    return compile_op<void (*)(void*, int64_t, int64_t)>(gen_table_op());
}
