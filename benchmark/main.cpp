#include <iostream>

#include "bench.h"

#include "reffine/base/type.h"
#include "reffine/builder/reffiner.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

int main()
{
    TPCHQuery6 tpchq6(820454400, 852076800, 0.05f, 24.5f);
    auto start = std::chrono::high_resolution_clock::now();
    auto out = tpchq6.run();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    cout << "Out: " << out << endl;
    cout << "Time: " << duration.count() << endl;
}
