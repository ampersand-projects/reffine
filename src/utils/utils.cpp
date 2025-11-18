#include "reffine/utils/utils.h"

#include <arrow/api.h>
#include <arrow/csv/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/c/bridge.h>

static arrow::Result<shared_ptr<ArrowTable2>> _load_arrow_file(string filename, int64_t dim)
{
    ARROW_ASSIGN_OR_RAISE(auto file, arrow::io::ReadableFile::Open(
                filename, arrow::default_memory_pool()));

    ARROW_ASSIGN_OR_RAISE(auto ipc_reader, arrow::ipc::RecordBatchFileReader::Open(file));

    ARROW_ASSIGN_OR_RAISE(auto rbatch, ipc_reader->ReadRecordBatch(0));
    cout << "Input: " << endl << rbatch->ToString() << endl;

    auto tbl = make_shared<ArrowTable2>(dim);
    ARROW_RETURN_NOT_OK(arrow::ExportRecordBatch(*rbatch, tbl->array, tbl->schema));

    return tbl;
}

shared_ptr<ArrowTable2> load_arrow_file(string filename, int64_t dim)
{
    return _load_arrow_file(filename, dim).ValueOrDie();
}
