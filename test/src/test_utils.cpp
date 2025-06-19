#include "test_utils.h"

arrow::Result<reffine::ArrowTable> get_input_vector()
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

    return reffine::ArrowTable(schema, array);
}

std::string print_arrow_table(reffine::ArrowTable& tbl)
{
    auto& schema = tbl.schema;
    auto& array = tbl.array;
    auto res = *arrow::ImportRecordBatch(&array, &schema);

    return res->ToString();
}

CUfunction compile_kernel(std::shared_ptr<reffine::Func> fn)
{
    reffine::CanonPass::Build(fn);

    auto jit = reffine::ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("foo", jit->GetCtx());
    reffine::LLVMGen::Build(fn, *llmod);
    jit->Optimize(*llmod);

    auto cuda_engine = reffine::CudaEngine::Get();
    auto cuda_module = cuda_engine->Build(*llmod);
    return cuda_engine->Lookup(cuda_module, llmod->getName().str());
}
