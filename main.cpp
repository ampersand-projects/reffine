#include <iostream>
#include <memory>

#include "reffine/ir/node.h"
#include "reffine/ir/expr.h"
#include "reffine/ir/loop.h"
#include "reffine/base/type.h"
#include "reffine/pass/printer.h"

using namespace reffine;
using namespace std;

int main()
{
    auto input = make_shared<SymNode>("in", types::VECTOR<1>(types::INT32));
    auto output = make_shared<SymNode>("out", types::VECTOR<1>(types::INT32));

    // construct the loop
    auto loop = make_shared<Loop>(output);
    auto idx = make_shared<SymNode>("idx", types::IDX);
    auto one = make_shared<Const>(BaseType::IDX, 1);
    loop->idx_inits[idx] = one;
    loop->idx_incrs[idx] = make_shared<Add>(idx, one);

    loop->body_cond = make_shared<Const>(BaseType::BOOL, 1);

    auto ten = make_shared<Const>(BaseType::IDX, 10);
    loop->exit_cond = make_shared<LessThan>(idx, ten);

    auto read = make_shared<Read>(input, idx);
    auto two = make_shared<Const>(BaseType::INT32, 2);
    auto add_two = make_shared<Add>(read, two);
    loop->body = make_shared<PushBack>(output, add_two);

    auto loop_sym = make_shared<SymNode>("loop", loop);
    
    // construct the function
    auto inputs = vector<Sym>{input, output};
    auto loop_fn = make_shared<Func>("query", loop_sym, inputs);
    loop_fn->tbl[loop_sym] = loop;

    cout << IRPrinter::Build(loop_fn);

    return 0;
}
