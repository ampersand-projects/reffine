#include <arrow/api.h>
#include <arrow/c/bridge.h>
#include <arrow/csv/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <arrow/result.h>
#include <arrow/status.h>

#include <iostream>

#include "bench.h"
#include "reffine/base/type.h"
#include "reffine/builder/reffiner.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

int main()
{
    TPCHQuery3 bench;
    auto start = std::chrono::high_resolution_clock::now();
    auto out = bench.run();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    cout << "Time: " << duration.count() << endl;

    auto out_res =
        arrow::ImportRecordBatch(out->array, out->schema).ValueOrDie();
    cout << "Output: " << endl << out_res->ToString() << endl;
}
