#include <iostream>

#include "bench.h"

#include "reffine/base/type.h"
#include "reffine/builder/reffiner.h"

using namespace std;
using namespace reffine;
using namespace reffine::reffiner;

int main()
{
    auto zero = _idx(0);
    cout << zero->str() << endl;
    return 0;
}
