#include "test_utils.h"

arrow::Result<reffine::ArrowTable> get_input_vector()
{
    ARROW_ASSIGN_OR_RAISE(
        auto infile, arrow::io::ReadableFile::Open(
                         "../../students.arrow", arrow::default_memory_pool()));
    ARROW_ASSIGN_OR_RAISE(auto ipc_reader,
                          arrow::ipc::RecordBatchFileReader::Open(infile));
    ARROW_ASSIGN_OR_RAISE(auto rbatch, ipc_reader->ReadRecordBatch(0));

    reffine::ArrowTable tbl("input", 0);
    ARROW_RETURN_NOT_OK(arrow::ExportRecordBatch(*rbatch, tbl.array.get(), tbl.schema.get()));

    return std::move(tbl);
}

std::string print_arrow_table(reffine::ArrowTable& tbl)
{
    auto res = *arrow::ImportRecordBatch(tbl.array.get(), tbl.schema.get());

    return res->ToString();
}
