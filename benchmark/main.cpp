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
}
