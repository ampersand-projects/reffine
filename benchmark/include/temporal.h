#include "reffine/builder/reffiner.h"
#include "reffine/utils/utils.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

struct AlgoTrading {
    using QueryFnTy = void (*)(ArrowTable**, ArrowTable*);

    shared_ptr<ArrowTable2> stock_price;
    QueryFnTy query_fn;

    AlgoTrading()
    {
        this->stock_price = load_arrow_file("../benchmark/arrow_data/stock_price.arrow", 1);
        this->stock_price->build_index();
        this->query_fn = compile_op<QueryFnTy>(this->build_op());
    }

    shared_ptr<Func> build_op()
    {
        return nullptr;
    }

    ArrowTable* run()
    {
        return nullptr;
    }
};
