#include "test_utils.h"

arrow::Result<ArrowTable> get_input_vector()
{
    ARROW_ASSIGN_OR_RAISE(
        auto infile, arrow::io::ReadableFile::Open(
                         "../../students.arrow", arrow::default_memory_pool()));
    ARROW_ASSIGN_OR_RAISE(auto ipc_reader,
                          arrow::ipc::RecordBatchFileReader::Open(infile));
    ARROW_ASSIGN_OR_RAISE(auto rbatch, ipc_reader->ReadRecordBatch(0));

    ArrowSchema schema;
    ArrowArray array;
    ARROW_RETURN_NOT_OK(arrow::ExportRecordBatch(*rbatch, &array, &schema));

    return ArrowTable(schema, array);
}

std::string print_output_vector(ArrowSchema schema, ArrowArray array)
{
    auto res = *arrow::ImportRecordBatch(&array, &schema);
    return res->ToString();
}
