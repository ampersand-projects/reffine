#include "test_utils.h"

arrow::Result<std::shared_ptr<reffine::ArrowTable2>> get_input_vector()
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
