#include <iostream>
#include <memory>

#include "reffine/ir/node.h"
#include "reffine/ir/expr.h"
#include "reffine/ir/loop.h"
#include "reffine/base/type.h"
#include "reffine/pass/printer.h"
#include "reffine/pass/llvmgen.h"
#include "reffine/engine/engine.h"

using namespace reffine;
using namespace std;

shared_ptr<Func> reffine_fn()
{
    auto input = make_shared<SymNode>("in", types::VECTOR<1>({types::INT32, types::INT32}));
    auto output = make_shared<SymNode>("out", types::VECTOR<1>({types::INT32, types::INT32}));

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

    return loop_fn;
}

shared_ptr<Func> simple_fn()
{
    auto c = make_shared<Const>(BaseType::INT32, 123);
    auto foo_fn = make_shared<Func>("foo", c, vector<Sym>{});
    return foo_fn;
}

int main()
{
    auto fn = simple_fn();
    cout << IRPrinter::Build(fn);

    auto jit = ExecEngine::Get();
    auto llmod = make_unique<llvm::Module>("test", jit->GetCtx());
    LLVMGen::Build(fn, *llmod);
    cout << IRPrinter::Build(*llmod);

    jit->AddModule(std::move(llmod));
    auto query_fn = jit->Lookup<int (*)()>(fn->name);

    cout << "Result: " << query_fn() << endl;

    return 0;
}
